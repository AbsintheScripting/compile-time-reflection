#include "CTaskScheduler.h"

#include <vector>

// take uint64_t, so we can use 64-bit pointers as unique id in the graph builder
#ifndef ENTT_ID_TYPE
#define ENTT_ID_TYPE std::uint64_t
#endif
// defined id type before include
#include <include/entt/src/entt/graph/flow.hpp>

#include "MetaResourceVisitor.hpp"

void CTaskScheduler::OrderAndExecuteTasks(std::queue<std::shared_ptr<ITask>> task_queue)
{
	// build task flow with entt
	entt::flow builder{};

	std::vector<std::shared_ptr<ITask>> taskList;
	taskList.reserve(task_queue.size());

	// lambda for std::apply
	auto registerResources = [&]<Meta::member_resource_access... Ts>(const Ts&... tuple_args)
	{
		((tuple_args.ACCESS_MODE == Meta::EResourceAccessMode::WRITE
			  ? builder.rw(tuple_args.GetHashCode())
			  : builder.ro(tuple_args.GetHashCode()))
			, ...);
	};

	while (!task_queue.empty())
	{
		taskList.push_back(std::move(task_queue.front()));
		task_queue.pop();

		auto taskId = reinterpret_cast<entt::id_type>(
			static_cast<void*>(taskList.back().get()) // <- use pointer as uid
		);
		builder.bind(taskId);

		std::any taskResources = taskList.back()->GetResources();
		TResourceVisitor::VisitAny(
			taskResources,
			[&]<typename T>(std::tuple<T> resources_tuple)
			{
				if constexpr (Meta::method_resources<T>)
				{
					// resource list as std::tuple
					constexpr auto filteredResources = T::GetFilteredResources();
					std::apply(registerResources, filteredResources);
				}
			}
		);
	}

	// build taskQueue main
	std::queue<size_t> scheduledTasks;

	// get first tasks
	const entt::adjacency_matrix<entt::directed_tag> graph = builder.graph();
	for (size_t&& vertex : graph.vertices())
		// vertex has no in_edges this means that this vertex is a main vertex
		if (auto inEdges = graph.in_edges(vertex); inEdges.begin() == inEdges.end())
			scheduledTasks.push(vertex);

	// hold references to all async threads until this vector goes out of scope
	std::vector<TAsyncTaskPtr> executedTasks(graph.size());
	// execute all tasks in a certain order
	while (!scheduledTasks.empty())
	{
		// get current task and remove from queue
		const size_t taskVertex = scheduledTasks.front();
		scheduledTasks.pop();

		TAsyncTaskPtr pFuture = std::make_shared<TAsyncTask>();

		// save reference to std::future beyond this while loop
		executedTasks.at(taskVertex) = pFuture;
		// list of async threads from parents, moved into lambda capture
		std::deque<std::weak_ptr<TAsyncTask>> parentTasks;
		for (auto&& [parent, child] : graph.in_edges(taskVertex))
			if (const auto& pParentFuture = executedTasks.at(parent))
				parentTasks.emplace_back(pParentFuture);

		// start async thread to do the work, thread is managed by OS
		*pFuture = std::async(
			std::launch::async,
			[pTask = taskList.at(taskVertex).get(), parentTasks = std::move(parentTasks)]()
			{
				// wait for all parent tasks
				for (auto&& pParentTask : parentTasks)
					if (const auto& pParentFuture = pParentTask.lock())
						pParentFuture->wait();

				// finally do the task
				pTask->DoTask();
			}
		);

		// add all children of current vertex to the task queue
		for (auto&& [parent, child] : graph.out_edges(taskVertex))
			scheduledTasks.push(child);
	}

	// wait for all started threads,
	// so in the next tick the script-thread doesn't start
	// before the last async thread has finished execution
	for (auto&& pTask : executedTasks)
		if (pTask)
			pTask->wait();
}
