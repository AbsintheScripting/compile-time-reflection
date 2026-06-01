#pragma once
#include <algorithm>
#include <array>
#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <Meta.hpp>
#include <queue>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace Meta
{
	/*
	 * ####################################
	 * priority
	 * ####################################
	 */

	using TPriority = int32_t;

	// Named anchors on a free integer axis so callers can introduce their own priority levels
	// (e.g. `constexpr Meta::TPriority Movement = 800;`)
	// without modifying the scheduler.
	struct EPriority
	{
		static constexpr TPriority Lowest = 0;
		static constexpr TPriority Low = 100;
		static constexpr TPriority Normal = 500;
		static constexpr TPriority High = 900;
		static constexpr TPriority Critical = 1000;
	};

	/*
	 * ####################################
	 * conflict primitives
	 * ####################################
	 */

	/**
	 * \brief Two resource accesses conflict if and only if they refer to the same member
	 *        and at least one side writes. Same member with both sides reading is safe
	 *        (concurrent readers are allowed).
	 */
	template <resource_access L, resource_access R>
	consteval bool resources_conflict()
	{
		// MEMBER_INFO is a std::meta::info -- comparing two of them yields true
		// only when they refer to the exact same reflected member declaration.
		// If the members differ, the accesses are unrelated and never conflict.
		if (L::MEMBER_INFO != R::MEMBER_INFO)
			return false;

		// Same member: only a conflict when at least one side intends to write.
		// Two reads can run concurrently, so READ + READ is not a conflict.
		return L::ACCESS_MODE == EResourceAccessMode::WRITE
			|| R::ACCESS_MODE == EResourceAccessMode::WRITE;
	}

	template <typename LeftTuple, typename RightTuple>
	struct CAnyPairConflicts;

	template <resource_access... Ls, resource_access... Rs>
	struct CAnyPairConflicts<std::tuple<Ls...>, std::tuple<Rs...>>
	{
		// For a single resource L from the left side, returns true when L conflicts with
		// any resource on the right side. This is a fold over the right-hand pack with logical OR.
		template <resource_access L>
		static consteval bool LHitsAny()
		{
			return (resources_conflict<L, Rs>() || ...);
		}

		// Returns true when ANY pair drawn from the Cartesian product of the
		// left tuple and right tuple conflicts. The structure is a nested OR-fold:
		// for each L in Ls, check whether L hits anything in Rs.
		static consteval bool Value()
		{
			// An empty fold over `|| ...` would produce false,
			// which happens to be the right answer here
			// (an empty resource list cannot conflict with anything),
			// but a fold over an empty pack with non-empty operator behaviour is fragile in some compilers,
			// so make the empty case explicit.
			if constexpr (sizeof...(Ls) == 0 || sizeof...(Rs) == 0)
				return false;
			else
				return (LHitsAny<Ls>() || ...);
		}
	};

	template <typename LeftTuple, typename RightTuple>
	consteval bool any_pair_conflicts()
	{
		return CAnyPairConflicts<LeftTuple, RightTuple>::Value();
	}

	/**
	 * \brief Two methods conflict if and only if their filtered resource tuples share a member
	 *        accessed with at least one write. The CNoResources sentinel touches the opaque
	 *        CNoType::noResource member which no real method references,
	 *        so it trivially conflicts with nothing -- including itself, since two reads do not conflict.
	 */
	template <method_resources Left, method_resources Right>
	consteval bool methods_conflict()
	{
		// GetFilteredResources() returns a std::tuple<CResourceAccess...> with duplicates removed
		// and read-when-write-exists collapsed to write.
		// We compare those two tuples for any conflicting pair.
		return any_pair_conflicts<
			decltype(Left::GetFilteredResources()),
			decltype(Right::GetFilteredResources())>();
	}

	/*
	 * ################################################
	 * scheduler traits -- precomputed conflict matrix
	 * ################################################
	 */

	template <typename MethodList>
	struct CSchedulerTraits;

	/**
	 * \brief Compile-time conflict matrix over a CMethodResourcesList.
	 *        Use `CSchedulerTraits<GLOBAL_METHOD_RESOURCE_LIST>` against the final registry
	 *        produced by including all `*.meta.h` headers.
	 */
	template <method_resources... Methods>
	struct CSchedulerTraits<CMethodResourcesList<Methods...>>
	{
		static constexpr size_t COUNT = sizeof...(Methods);
		using TBits = std::bitset<COUNT>;

		/**
		 * \brief Returns the position of `Method` in the registry, or COUNT
		 *        when the type is not registered. COUNT acts as a sentinel "not found" value
		 *        so callers can distinguish a real index from a missing one without exceptions.
		 */
		template <typename Method>
		static consteval size_t IndexOf()
		{
			// Linear scan over the parameter pack. `idx` is advanced once per type in the pack
			// via a fold expression; the lambda body runs exactly sizeof...(Methods) times.
			// We capture the first match by gating the assignment on `found == COUNT`
			// so subsequent matches (which should not occur -- types in the registry are unique)
			// do not overwrite the result.
			size_t idx = 0;
			size_t found = COUNT;
			auto step = [&]<typename M>(std::type_identity<M>)
			{
				if (found == COUNT && std::is_same_v<Method, M>)
					found = idx;
				++idx;
			};
			// Fold expression unrolls the loop at compile time:
			// one call per type in the Methods... pack, in declaration order.
			(step(std::type_identity<Methods>{}), ...);
			return found;
		}

		/**
		 * \brief Conflict matrix indexed by registry position.
		 *        Row R has bit B set if and only if method R conflicts with method B.
		 *        The matrix is symmetric (conflict is a symmetric relation)
		 *        and the diagonal reflects self-conflict.
		 *        True for any method that writes to at least one of its own resources.
		 *        Built once at compile time, then used as an O(1) lookup at runtime by the scheduler.
		 */
		static constexpr std::array<TBits, COUNT> CONFLICT_ROWS = []
		{
			// One bitset per registered method; default-initialised to all zeros.
			std::array<TBits, COUNT> rows{};

			// Outer iteration: pick a method `M` (the "row" method)
			// and walk every method in the registry to fill that row's bits.
			// We use a stateful lambda + fold to advance `rowIndex` once per outer step.
			size_t rowIndex = 0;
			auto fillRow = [&]<typename M>(std::type_identity<M>)
			{
				// Inner iteration: for each column method in the registry,
				// ask whether (M, column) conflict and set the bit accordingly.
				// `bit++` advances the column index in lockstep with the fold.
				size_t bit = 0;
				((rows[rowIndex].set(bit++, methods_conflict<M, Methods>())), ...);
				++rowIndex;
			};

			// Drive the outer iteration: one call per method in the pack.
			((fillRow(std::type_identity<Methods>{})), ...);
			return rows;
		}();
	};

	/*
	 * ####################################
	 * scheduled task / schedule output
	 * ####################################
	 */

	struct CScheduledTask
	{
		size_t inputIndex; // index into the input span
		std::vector<size_t> parents; // indices into the schedule that must finish first
	};

	using TSchedule = std::vector<CScheduledTask>;

	/*
	 * ####################################
	 * scheduler -- generic over pointer-like task handles
	 * ####################################
	 */

	template <typename T>
	concept scheduler_task_ptr = requires(const T& p)
	{
		{ p->GetPriority() } -> std::convertible_to<TPriority>;
		p->GetMethodMask();
		p->GetConflictMask();
		p->DoTask();
	};

	class CScheduler
	{
	public:
		/**
		 * \brief Build a dependency schedule from a queue of task handles.
		 *        Tasks are sorted by priority (descending, stable);
		 *        each task records the prior tasks in the schedule it conflicts with as its parents.
		 *        Transitive edges are kept on purpose -- the executor waits on parent futures
		 *        and redundant edges are cheap, while transitive reduction is O(n³).
		 */
		template <scheduler_task_ptr TaskPtr>
		static TSchedule Schedule(std::span<const TaskPtr> tasks)
		{
			const size_t numTasks = tasks.size();

			// Step 1: build an index permutation that visits the input in priority-descending order.
			// We sort indices instead of moving the task handles so the caller's storage stays untouched
			// and so we can still reference the original positions in CScheduledTask.
			std::vector<size_t> order(numTasks);
			for (size_t idx = 0; idx < numTasks; ++idx)
				order[idx] = idx;

			// stable_sort preserves submission order between equal-priority tasks.
			// This matters for fairness: two equally critical packets stay in arrival order,
			// which is what the game loop expects.
			std::stable_sort(order.begin(), order.end(),
			                 [&](size_t lhs, size_t rhs)
			                 {
				                 return tasks[lhs]->GetPriority() > tasks[rhs]->GetPriority();
			                 });

			// Step 2: walk the sorted order and emit a CScheduledTask for each task.
			// The schedule grows in topological order by construction:
			// every parent we ever record was emitted earlier in this loop.
			TSchedule schedule;
			schedule.reserve(numTasks);

			for (size_t srcIdx : order)
			{
				// The current task's compile-time CONFLICT_MASK has bit B set
				// when this task conflicts with the B-th registered method.
				const auto& taskConflict = tasks[srcIdx]->GetConflictMask();

				// Scan everything already in the schedule.
				// A previously-emitted task is a parent of the current one
				// exactly when its METHOD bits overlap with our CONFLICT bits;
				// that is precisely the runtime question we precomputed the matrix to answer in O(1).
				std::vector<size_t> parents;
				for (size_t scheduleIndex = 0; scheduleIndex < schedule.size(); ++scheduleIndex)
				{
					const size_t prevSrc = schedule[scheduleIndex].inputIndex;
					// (conflictMask & methodMask).any()
					// -> a single bitset AND plus zero-check.
					if ((taskConflict & tasks[prevSrc]->GetMethodMask()).any())
						parents.push_back(scheduleIndex);
				}

				// Note: we deliberately keep transitive edges.
				// If A -> B and B -> C, we also record A -> C when the masks overlap.
				// Removing those would cost O(n^3); leaving them in costs an extra wait()
				// on an already-completed future, which is free.
				schedule.push_back(CScheduledTask{srcIdx, std::move(parents)});
			}

			return schedule;
		}

		/**
		 * \brief Execute a previously built schedule.
		 *        Each task starts on a fresh std::async thread but waits on the futures of its parent tasks
		 *        before invoking DoTask(). Blocks until every task in the schedule has finished.
		 *
		 * \tparam ShouldContinueFn nullary callable returning bool; called inside each worker
		 *                          before executing the task body; lets the caller abort mid-tick.
		 */
		template <scheduler_task_ptr TaskPtr, typename ShouldContinueFn>
		static void Execute(const TSchedule& schedule,
		                    std::span<const TaskPtr> tasks,
		                    ShouldContinueFn&& isRunning)
		{
			using TFuture = std::future<void>;
			using TFuturePtr = std::shared_ptr<TFuture>;

			// One future slot per scheduled task. We wrap each std::future in a shared_ptr
			// so that worker lambdas can hold a weak_ptr to their parents;
			// the future objects must outlive the lambdas that wait on them,
			// and shared_ptr makes the lifetime explicit.
			std::vector<TFuturePtr> futures(schedule.size());

			for (size_t scheduleIndex = 0; scheduleIndex < schedule.size(); ++scheduleIndex)
			{
				// Unpack: which input task does this slot run,
				// and which already-launched slots must finish before it starts?
				const auto& [taskIndex, parents] = schedule[scheduleIndex];

				// Collect weak_ptrs to the parent futures.
				// Using weak_ptr (not shared_ptr) avoids extending the parent future's lifetime past `futures` itself;
				// we keep the strong ownership in the `futures` vector only, and the lambdas just observe.
				std::vector<std::weak_ptr<TFuture>> parentFutures;
				parentFutures.reserve(parents.size());
				for (const size_t parentIndex : parents)
					parentFutures.emplace_back(futures[parentIndex]);

				// Allocate this slot's future BEFORE launching async,
				// so that any later sibling that lists us as a parent
				// can already observe a valid weak_ptr in `futures[scheduleIndex]`.
				TFuturePtr pFuture = std::make_shared<TFuture>();
				futures[scheduleIndex] = pFuture;

				// Snapshot the task handle for capture;
				// TaskPtr can be a raw pointer or a shared_ptr -- either way it's cheap to copy.
				TaskPtr taskHandle = tasks[taskIndex];

				// Fire the worker.
				// std::launch::async forces a new OS thread (no deferred execution), which we need:
				// deferred execution would only run when the parent waits on the future,
				// and we want true parallelism between non-conflicting tasks.
				*pFuture = std::async(
					std::launch::async,
					[taskHandle, parents = std::move(parentFutures), &isRunning]() mutable
					{
						// Cooperative cancellation:
						// if the host (e.g. the game server) has stopped running,
						// abandon the tick without invoking the task body.
						if (!isRunning())
							return;

						// Wait on every parent future to finish before we touch any shared resource.
						// Locking a weak_ptr is safe even if the parent has already been destroyed;
						// But the `futures` vector outlives this lambda because we block at the end of Execute.
						for (auto& weak : parents)
							if (const auto p = weak.lock())
								p->wait();

						// All parents done -- safe to run the task body.
						taskHandle->DoTask();
					}
				);
			}

			// Tick barrier: do not return until every async worker has finished.
			// The next tick must start from a quiescent state,
			// so the executor's contract is "synchronous from the caller's view".
			for (auto& future : futures)
				if (future)
					future->wait();
		}

		/**
		 * \brief Convenience helper: build then execute.
		 *        The default isRunning check always returns true.
		 */
		template <scheduler_task_ptr TaskPtr>
		static void OrderAndExecute(std::span<const TaskPtr> tasks)
		{
			const TSchedule schedule = Schedule<TaskPtr>(tasks);
			Execute<TaskPtr>(schedule, tasks, []
			{
				return true;
			});
		}
	};
}
