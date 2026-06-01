/**
 * The idea is to construct a simple reflection tool to reflect on
 * resource usage and how they are accessed (read or write mode).
 * It is basically a list of all accessed resources of one routine,
 * including accessed resources of all sub-routines.
 * So when I define a task for a multithreaded system, I only list accessed
 * resources and called functions without the need to manually go into every
 * function to check on used resources.
 */

#include <any>
#include <functional>
#include <iostream>
#include <Meta.hpp>
#include <MetaResourceVisitor.hpp>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "MetaResourceList.h"
#include "MetaTask.hpp"

int main()
{
	/***************
	 * Static tests
	 ***************/
	// check global registered resources
	static_assert(std::tuple_size_v<Meta::TGlobalResourceList> > 0);
	// check members via C++26 reflection
	using TMode = Meta::EResourceAccessMode;
	static_assert(std::is_same_v<typename [:std::meta::type_of(CFoo::CMeta::TNumber<TMode::READ>::MEMBER_INFO):], int>);
	static_assert(std::meta::identifier_of(CFoo::CMeta::TNumber<TMode::READ>::MEMBER_INFO) == std::string_view("number"));
	static_assert(std::is_same_v<typename [:std::meta::type_of(CBar::CMeta::TSomeNumber<TMode::READ>::MEMBER_INFO):], int>);
	static_assert(std::is_same_v<typename [:std::meta::type_of(CBar::CMeta::TSomeString<TMode::READ>::MEMBER_INFO):],std::string>);
	static_assert(std::is_same_v<typename [:std::meta::type_of(CBar::CMeta::TAnotherString<TMode::READ>::MEMBER_INFO):],std::string>);
	static_assert(
		std::meta::identifier_of(CBar::CMeta::TAnotherString<TMode::READ>::MEMBER_INFO)
		== std::string_view("anotherString")
	);

	// define aliases to check
	using TSomeNumberWrite = CBar::CMeta::TSomeNumber<TMode::WRITE>;
	using TSomeStringRead = CBar::CMeta::TSomeString<TMode::READ>;
	using TSomeStringWrite = CBar::CMeta::TSomeString<TMode::WRITE>;
	using TAnotherStringWrite = CBar::CMeta::TAnotherString<TMode::WRITE>;
	using TSomeMethodResources = Meta::CMethodResources<TSomeNumberWrite, TSomeStringWrite>;
	using TFooBarNumRead = IFooBar::CMeta::TFooBarNum<TMode::READ>;
	using TFooBarNumWrite = IFooBar::CMeta::TFooBarNum<TMode::WRITE>;
	using TBarFooNumRead = CBarFoo::CMeta::TBarFooNum<TMode::READ>;
	using TOtherFooBarNumWrite = CFooBar::CMeta::TOtherFooBarNum<TMode::WRITE>;

	// check TUniqueTypes
	static_assert(std::is_same_v<Meta::TUniqueTypes<TSomeStringRead, TSomeStringRead, TSomeStringWrite>,
	                             // order is inverse, filters out 1x TSomeStringRead
	                             Meta::CFilteredUniqueTypeList<TSomeStringWrite, TSomeStringRead>>);
	// check concept exist_write_access
	static_assert(Meta::exist_write_access<TSomeStringRead, TSomeStringWrite>);
	// check TResourceTypes
	static_assert(std::is_same_v<Meta::TResourceTypes<TSomeStringRead, TSomeStringWrite>,
	                             Meta::CFilteredResourceTypeList<TSomeStringWrite>>); // filters out TSomeStringRead
	static_assert(std::is_same_v<Meta::TResourceTypes<TSomeStringWrite, TSomeStringRead>::TTypes, // different order
	                             std::tuple<TSomeStringWrite>>); // filters out TSomeStringRead
	static_assert(std::is_same_v<decltype(
		                             Meta::CMethodResources<TSomeStringRead, TSomeStringWrite>
		                             ::GetFilteredResources()
	                             ),
	                             std::tuple<TSomeStringWrite>>); // filters out TSomeStringRead
	static_assert(std::is_same_v<decltype(
		                             Meta::CMethodResources<TSomeStringWrite, TSomeStringRead> // different order
		                             ::GetFilteredResources()
	                             ),
	                             std::tuple<TSomeStringWrite>>); // filters out TSomeStringRead
	// check Methods have the right resources
	static_assert(std::is_same_v<decltype(CFoo::CMeta::TMethodA::GetFilteredResources()),
	                             std::tuple<CFoo::CMeta::TNumber<TMode::WRITE>,
	                                        TSomeNumberWrite,
	                                        TSomeStringRead>>);
	static_assert(std::is_same_v<decltype(CFoo::CMeta::TMethodB::GetFilteredResources()),
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite>>);
	static_assert(std::is_same_v<decltype(CFoo::CMeta::TMethodC::GetFilteredResources()),
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite,
	                                        TAnotherStringWrite>>);
	static_assert(std::is_same_v<decltype(IFooBar::CMeta::TAbstractMethod::GetFilteredResources()),
	                             std::tuple<TFooBarNumRead,
	                                        TBarFooNumRead>>);
	static_assert(std::is_same_v<decltype(IFooBar::CMeta::TVirtualMethod::GetFilteredResources()),
	                             std::tuple<TOtherFooBarNumWrite,
	                                        TFooBarNumWrite>>);
	// check recursion (by mixing a CMethodResources' param pack with a CMethodResources and a CMemberResourceAccess)
	using TRecursiveMethodResources = Meta::CMethodResources<TSomeMethodResources, TSomeStringRead>;
	static_assert(std::is_same_v<decltype(TRecursiveMethodResources::GetResources()),
	                             std::tuple<TSomeNumberWrite, TSomeStringWrite, TSomeStringRead>>);
	static_assert(std::is_same_v<decltype(TRecursiveMethodResources::GetFilteredResources()),
	                             std::tuple<TSomeNumberWrite, TSomeStringWrite>>); // filters out TSomeStringRead

	// Retrieve filtered resources
	// type: std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,  // TSomeNumberWrite
	//                  CSomeString<EResourceAccessMode::WRITE>>  // TSomeStringWrite
	constexpr auto barMethod = Meta::Bar::MMethod::GetFilteredResources();
	static_assert(std::is_same_v<std::decay_t<decltype(barMethod)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite>>);
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(barMethod).ACCESS_MODE == TMode::WRITE
	);
	static_assert( // CSomeString<EResourceAccessMode::WRITE>
		std::get<1>(barMethod).ACCESS_MODE == TMode::WRITE
	);
	// input types:    std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,     // TSomeNumberWrite
	//                            CSomeString<EResourceAccessMode::WRITE>,     // TSomeStringWrite
	//                            CSomeString<EResourceAccessMode::READ>,      // TSomeStringRead
	//                            CSomeString<EResourceAccessMode::READ>,      // TSomeStringRead
	//                            CAnotherString<EResourceAccessMode::WRITE>>  // TAnotherStringWrite
	// ---------------------------------------------------------------------------------------------
	// filtered types: std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,     // TSomeNumberWrite
	//                            CSomeString<EResourceAccessMode::WRITE>,     // TSomeStringWrite
	//                            CAnotherString<EResourceAccessMode::WRITE>>  // TAnotherStringWrite
	constexpr auto fooMethodC = Meta::Foo::MMethodC::GetFilteredResources();
	static_assert(std::is_same_v<std::decay_t<decltype(fooMethodC)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite,
	                                        TAnotherStringWrite>>);
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(fooMethodC).ACCESS_MODE == TMode::WRITE
	);

	// Use Meta helper method to check resources
	static_assert(
		Meta::is_same_method_resources(CFoo::CMeta::TMethodA{},
		                               Meta::CMethodResources<CFoo::CMeta::TNumber<TMode::WRITE>,
		                                                      TSomeNumberWrite,
		                                                      TSomeStringRead>{})
	);
	// different orders
	static_assert(
		Meta::is_same_method_resources(CFoo::CMeta::TMethodA{},
		                               Meta::CMethodResources<TSomeNumberWrite,
		                                                      CFoo::CMeta::TNumber<TMode::WRITE>,
		                                                      TSomeStringRead>{})
	);
	static_assert(
		Meta::is_same_method_resources(CFoo::CMeta::TMethodA{},
		                               Meta::CMethodResources<TSomeNumberWrite,
		                                                      TSomeStringRead,
		                                                      CFoo::CMeta::TNumber<TMode::WRITE>>{})
	);

	// Check annotations
	using TAnnotationOfFooMethodA = Meta::TAnnotation<^^CFoo::MethodA>;
	using TAnnotationOfFooMethodB = Meta::TAnnotation<^^CFoo::MethodB>;
	using TAnnotationOfFooMethodC = Meta::TAnnotation<^^CFoo::MethodC>;
	using TAnnotationOfFooBarVirtualMethod = Meta::TAnnotation<^^IFooBar::VirtualMethod>;
	static_assert(std::is_same_v<TAnnotationOfFooMethodA, CFoo::CMeta::TMethodA>);
	static_assert(std::is_same_v<TAnnotationOfFooMethodB, CFoo::CMeta::TMethodB>);
	static_assert(std::is_same_v<TAnnotationOfFooMethodC, CFoo::CMeta::TMethodC>);
	static_assert(std::is_same_v<TAnnotationOfFooBarVirtualMethod, IFooBar::CMeta::TVirtualMethod>);

	// Compile-time conflict checks against the example tasks (A/B/C/D/E).
	static_assert(Meta::methods_conflict<Meta::Foo::MReadSomeString, Meta::Bar::MMethod>()); // A vs B
	static_assert(!Meta::methods_conflict<Meta::Foo::MReadSomeString, Meta::Bar::MSetAnotherString>()); // A vs C
	static_assert(!Meta::methods_conflict<Meta::Bar::MMethod, Meta::Bar::MSetAnotherString>()); // B vs C
	static_assert(!Meta::methods_conflict<Meta::CNoResources, Meta::Foo::MReadSomeString>()); // D vs A
	static_assert(Meta::methods_conflict<Meta::Foo::MMethodA, Meta::Foo::MReadSomeString>()); // E (Foo::number) vs A
	static_assert(Meta::methods_conflict<Meta::Foo::MMethodA, Meta::Bar::MMethod>()); // E vs B
	static_assert(Meta::methods_conflict<Meta::Foo::MMethodC, Meta::Bar::MSetAnotherString>()); // E vs C
	// CNoResources never conflicts with anything, including itself
	static_assert(!Meta::methods_conflict<Meta::CNoResources, Meta::CNoResources>());

	/***************
	 * Runtime tests
	 ***************/

	// Create our test objects
	auto myFoo = std::make_unique<CFoo>();
	auto myBar = std::make_unique<CBar>();

	// Create some tasks
	using namespace std::this_thread; // sleep_for, sleep_until
	using namespace std::chrono; // nanoseconds, system_clock, seconds

	auto sleepDuration = nanoseconds(1000);
	auto sleepDurationD = seconds(1);
	// Task A
	// Write accesses: Foo::number
	// Read accesses: Bar::someString
	std::function funA = [&]()
	{
		std::cout << "Execute function A\n";
		sleep_for(sleepDuration);
		myFoo->ReadSomeString(*myBar);
		sleep_for(sleepDuration);
		std::cout << "Function A end\n";
	};
	// type std::tuple< struct Meta::Bar::CSomeString<0>, struct Meta::Foo::CNumber<1> >
	using TTaskA = Meta::CTask<Meta::Foo::MReadSomeString>;
	auto taskA = std::make_shared<TTaskA>(std::move(funA));

	// Task B
	// Write accesses: Bar::someNumber and Bar::someString
	// Read accesses: none
	std::function funB = [&]()
	{
		std::cout << "Execute function B\n";
		sleep_for(sleepDuration);
		myBar->Method();
		sleep_for(sleepDuration);
		std::cout << "Function B end\n";
	};
	// type std::tuple< struct Meta::Bar::CSomeString<1>, struct Meta::Bar::CSomeNumber<1> >
	using TTaskB = Meta::CTask<Meta::Bar::MMethod>;
	auto taskB = std::make_shared<TTaskB>(std::move(funB));

	// Task C
	// Write accesses: Bar::anotherString
	// Read accesses: none
	std::function funC = [&]()
	{
		std::cout << "Execute function C\n";
		sleep_for(sleepDuration);
		myBar->SetAnotherString("Test");
		sleep_for(sleepDuration);
		std::cout << "Function C end\n";
	};
	// type std::tuple< struct Meta::Bar::CAnotherString<1> >
	using TTaskC = Meta::CTask<Meta::Bar::MSetAnotherString>;
	auto taskC = std::make_shared<TTaskC>(std::move(funC));

	// Task D
	// Write accesses: none
	// Read accesses: none
	std::function funD = [&]()
	{
		std::cout << "Execute function D\n";
		sleep_for(sleepDurationD);
		std::cout << "Function D end\n";
	};
	// type std::tuple< struct Meta::CNoResource<0> >
	using TTaskD = Meta::CTask<Meta::CNoResources>;
	auto taskD = std::make_shared<TTaskD>(std::move(funD));

	// Task E
	// Write accesses: Foo::number, Bar::someNumber, Bar::anotherString, Bar::someString
	// Read accesses: none
	std::function funE = [&]()
	{
		std::cout << "Execute function E\n";
		sleep_for(sleepDuration);
		myFoo->MethodA(*myBar);
		myFoo->MethodB(*myBar);
		myFoo->MethodC(*myBar);
		sleep_for(sleepDuration);
		std::cout << "Function E end\n";
	};

	// type filtered std::tuple<struct Meta::Foo::CNumber<1>,struct Meta::Bar::CSomeNumber<1>,struct Meta::Bar::CSomeString<1>,struct Meta::Bar::CAnotherString<1> >
	using TTaskE = Meta::CTask<Meta::Foo::MMethodA, Meta::Foo::MMethodB, Meta::Foo::MMethodC>;
	auto taskE = std::make_shared<TTaskE>(std::move(funE));

	// Add tasks to our scheduler queue and task list
	// Conflicts:
	//   - taskA and taskB, because funA wants to read Bar::someString while funB tries to write it
	//   - taskE has conflicts with taskA, taskB and taskC, with write access to:
	//       Foo::number, Bar::someNumber, Bar::anotherString, Bar::someString
	//   - Task D has no conflicts and can run in parallel with all tasks
	std::queue<std::shared_ptr<Meta::ITask>> schedulerTaskQueue;
	schedulerTaskQueue.push(taskA);
	schedulerTaskQueue.push(taskB);
	schedulerTaskQueue.push(taskC);
	schedulerTaskQueue.push(taskD);
	schedulerTaskQueue.push(taskE);
	std::vector<std::shared_ptr<Meta::ITask>> tasks;
	tasks.push_back(taskA);
	tasks.push_back(taskB);
	tasks.push_back(taskC);
	tasks.push_back(taskD);
	tasks.push_back(taskE);

	// Print resources

	// Get the global resource list as tuple
	constexpr auto seqResourceTuple = std::make_index_sequence<std::tuple_size_v<Meta::TGlobalResourceList>>{};
	auto printTuple = [&]<typename Tuple, std::size_t... Is>(std::index_sequence<Is...>, Tuple tuple)
	{
		(
			(std::cout << typeid(std::tuple_element_t<Is, Tuple>).name() <<
				" (" << typeid(std::tuple_element_t<Is, Tuple>).hash_code() << ")" << std::endl
			),
			...
		);
	};
	// iterate over the sequence and print all global resources
	std::cout << "Meta::TResourceTuple types:" << std::endl;
	printTuple(seqResourceTuple, Meta::TGlobalResourceList{});

	std::cout << "" << std::endl;
	std::cout << "Task A types:" << std::endl;
	constexpr auto seqTaskATuple = std::make_index_sequence<std::tuple_size_v<TTaskA::TResources>>{};
	printTuple(seqTaskATuple, TTaskA::TResources{});
	std::cout << "Task B types:" << std::endl;
	constexpr auto seqTaskBTuple = std::make_index_sequence<std::tuple_size_v<TTaskB::TResources>>{};
	printTuple(seqTaskBTuple, TTaskB::TResources{});
	std::cout << "Task C types:" << std::endl;
	constexpr auto seqTaskCTuple = std::make_index_sequence<std::tuple_size_v<TTaskC::TResources>>{};
	printTuple(seqTaskCTuple, TTaskC::TResources{});
	std::cout << "Task D types:" << std::endl;
	constexpr auto seqTaskDTuple = std::make_index_sequence<std::tuple_size_v<TTaskD::TResources>>{};
	printTuple(seqTaskDTuple, TTaskD::TResources{});
	std::cout << "Task E types:" << std::endl;
	constexpr auto seqTaskETuple = std::make_index_sequence<std::tuple_size_v<TTaskE::TResources>>{};
	printTuple(seqTaskETuple, TTaskE::TResources{});
	std::cout << "Task E filtered resources:" << std::endl;
	constexpr auto filteredResources = TTaskE::GetFilteredResources();
	constexpr auto seqFilteredResources = std::make_index_sequence<std::tuple_size_v<decltype(filteredResources)>>{};
	printTuple(seqFilteredResources, filteredResources);
	std::cout << "" << std::endl;

	// Use resource visitor with global resource list
	using TResourceVisitor = Meta::CResourceVisitor<Meta::TGlobalResourceList>;

	std::cout << "Checking task types:" << std::endl;
	for (auto&& task : tasks)
	{
		std::any resources = task->GetMetaResources();
		std::type_index typeInfo = resources.type();
		std::cout << "Task: \t" << typeInfo.name() << " (" << typeInfo.hash_code() << ")" << std::endl;
		size_t numResources = task->GetNumResources();
		for (size_t idx = 0; idx < numResources; ++idx)
		{
			resources = task->GetMetaResource(idx);
			typeInfo = resources.type();
			std::cout << "\t" << (idx + 1) << ". check:\t" << typeInfo.name() << " (" << typeInfo.hash_code() << ")" <<
				std::endl;
			TResourceVisitor::VisitAny(
				resources,
				[&]<typename T>(std::tuple<T> resource_tuple)
				{
					if constexpr (Meta::method_resources<T>)
					{
						std::type_index taskType = typeid(T);
						constexpr auto resourcesTuple = T::GetFilteredResources(); // std::tuple<Resources...>
						std::type_index taskResourceTuple = typeid(resourcesTuple);
						std::cout << "\tFound type:\t"
							<< taskType.name() << " (" << taskType.hash_code() << ")"
							<< std::endl;
						std::cout << "\tResources:\t"
							<< taskResourceTuple.name() << " (" << taskResourceTuple.hash_code() << ")"
							<< std::endl;
					}
				}
			);
		}
	}

	// Schedule tasks
	// Task A and B execution shall not overlap, since they have a conflicting resource
	// Task E has conflicts with A, B and C
	// Task D has no conflicts and can run in parallel with all tasks
	const std::span<const std::shared_ptr<Meta::ITask>> tasksSpan{tasks};
	const Meta::TSchedule schedule = Meta::CScheduler::Schedule(tasksSpan);
	// Now execute the schedule
	std::cout << "" << std::endl;
	std::cout << "Executing tasks:" << std::endl;
	Meta::CScheduler::Execute(schedule, tasksSpan, []
	{
		return true;
	});
	// Expected if we start with task A:
	// 1. start A, C, D
	// 2. end A, C
	// 3. start B
	// 4. end B
	// 5. start E
	// 6. end E
	// 7. end D

	/***********************
	 * Priority demonstration
	 ***********************/

	// Pretty-printer for a Meta::TSchedule.
	// For each scheduled position it shows the user-visible label of the task running in that slot,
	// its priority, and the indices of the slots it has to wait for (its parents in the dependency DAG).
	auto printSchedule = [](const char* label,
	                        const Meta::TSchedule& scheduled_tasks,
	                        std::span<const std::shared_ptr<Meta::ITask>> tasks_span,
	                        const std::vector<const char*>& labels)
	{
		std::cout << '\n' << label << " schedule (priority desc):\n";
		// scheduleIdx walks the output order (already topologically sorted,
		// because the scheduler emits parents before children by construction).
		for (size_t scheduleIdx = 0; scheduleIdx < scheduled_tasks.size(); ++scheduleIdx)
		{
			// inputIndex maps back to the caller's tasks vector;
			// parents are indices into THIS schedule, not the input.
			const auto& [taskIndex, parents] = scheduled_tasks[scheduleIdx];
			std::cout << "  [" << scheduleIdx << "] " << labels[taskIndex]
				<< " (prio=" << tasks_span[taskIndex]->GetPriority() << ")"
				<< " parents={";
			// Comma-separate the parent indices.
			for (size_t parent = 0; parent < parents.size(); ++parent)
			{
				if (parent)
					std::cout << ", ";
				std::cout << parents[parent];
			}
			std::cout << "}\n";
		}
	};

	// Sanity print for the original A/B/C/D/E case. All five tasks share the default (lowest) priority,
	// so the schedule order matches submission order;
	// the interesting part is the parent links derived from the compile-time conflict matrix.
	{
		std::vector<std::shared_ptr<Meta::ITask>> abcde = {taskA, taskB, taskC, taskD, taskE};
		std::vector<const char*> labels = {"A", "B", "C", "D", "E"};
		std::span<const std::shared_ptr<Meta::ITask>> view{abcde};
		// Build the schedule once and print it;
		// the actual execution of this case already happened above through Meta::CTaskScheduler.
		const auto sched = Meta::CScheduler::Schedule(view);
		printSchedule("A/B/C/D/E", sched, view, labels);
	}

	// Scenario 1: two tasks with different priorities AND disjoint resources.
	// The high-priority task touches Bar::anotherString only;
	// the low-priority task touches Bar::someString (read) and Foo::number (write).
	// Since the member sets do not overlap, the bitset conflict check returns false
	// and neither task accumulates a parent in the schedule. The expected outcome is that both tasks
	// launch in parallel even though one is much higher priority;
	// priority controls ORDERING in the schedule, not artificial barriers between unrelated tasks.
	{
		std::cout << "\nScenario 1: non-conflicting low-prio + high-prio -> parents empty\n";

		// Low-priority task: simulates a routine read-only synchronisation.
		// Annotation Meta::Foo::MReadSomeString -> reads Bar::someString, writes Foo::number.
		auto lowPrio = std::make_shared<Meta::CTask<Meta::Foo::MReadSomeString>>(
			[]
			{
				std::cout << "low-prio (read someString) running\n";
			},
			Meta::EPriority::Low);

		// High-priority task: simulates "set another string" -- a write to Bar::anotherString and nothing else.
		auto highPrio = std::make_shared<Meta::CTask<Meta::Bar::MSetAnotherString>>(
			[]
			{
				std::cout << "high-prio (write anotherString) running\n";
			},
			Meta::EPriority::High);

		// Pack both tasks into a span the scheduler can read.
		// Using a vector of shared_ptr matches what the example app uses elsewhere;
		// the scheduler is templated on the pointer type and works just as well with raw ITask*
		std::vector<std::shared_ptr<Meta::ITask>> arr = {lowPrio, highPrio};
		std::vector<const char*> labels = {"low/readSomeString", "high/writeAnotherString"};
		std::span<const std::shared_ptr<Meta::ITask>> view{arr};

		// Build the schedule.
		// Internally: stable-sort by priority desc (highPrio first because 900 > 100),
		// then for each task scan the already-emitted entries for bitset overlap.
		// There is none here, so both end up with empty parent lists.
		const auto sched = Meta::CScheduler::Schedule(view);
		printSchedule("Scenario 1", sched, view, labels);

		// Self-check: the whole point of this scenario is
		// that the low-prio task does NOT accumulate the high-prio task as a parent.
		bool ok = sched.size() == 2
			&& sched[0].parents.empty()
			&& sched[1].parents.empty();
		std::cout << "  -> both independent: " << (ok ? "PASS" : "FAIL") << '\n';

		// Execute the schedule. With empty parent lists, both worker lambdas skip the parent-wait loop
		// and call DoTask() immediately on separate threads. The two "running" lines may print in either order.
		Meta::CScheduler::Execute(sched, view, []
		{
			return true;
		});
	}

	// Scenario 2: two tasks where the resource sets DO overlap on a member (Bar::someString)
	// and at least one side writes. The high-priority task writes someString; the low-priority task reads it.
	// That's a read-write conflict, so the scheduler must serialise them.
	// Because the high-prio task is sorted first, it ends up at schedule index 0 with no parents,
	// and the low-prio task ends up at index 1 with parents = {0}. This shows how priority and conflict combine:
	// priority breaks the tie about who runs first, conflict tells the scheduler that they cannot overlap.
	{
		std::cout << "\nScenario 2: conflicting low-prio + high-prio -> low waits for high\n";

		// Low-priority task: reads Bar::someString (annotation MReadSomeString).
		auto lowPrio = std::make_shared<Meta::CTask<Meta::Foo::MReadSomeString>>(
			[]
			{
				std::cout << "low-prio (read someString) running\n";
			},
			Meta::EPriority::Low);

		// High-priority task: writes Bar::someString (annotation MMethod also writes Bar::someNumber,
		// but only the someString overlap matters here).
		auto highPrio = std::make_shared<Meta::CTask<Meta::Bar::MMethod>>(
			[]
			{
				std::cout << "high-prio (write someString) running\n";
			},
			Meta::EPriority::High);

		// Same wiring as Scenario 1; only the resource overlap differs.
		std::vector<std::shared_ptr<Meta::ITask>> arr = {lowPrio, highPrio};
		std::vector<const char*> labels = {"low/readSomeString", "high/writeSomeString"};
		std::span<const std::shared_ptr<Meta::ITask>> view{arr};
		const auto sched = Meta::CScheduler::Schedule(view);
		printSchedule("Scenario 2", sched, view, labels);

		// Self-check: high must land at output index 0 (it was sorted first by priority),
		// low at index 1, and low's parent list must contain exactly the high-prio slot (index 0).
		// The high-prio slot itself has no parents because nothing was emitted before it.
		bool ok = sched.size() == 2
			&& sched[0].inputIndex == 1
			&& sched[0].parents.empty()
			&& sched[1].inputIndex == 0
			&& sched[1].parents.size() == 1
			&& sched[1].parents[0] == 0;
		std::cout << "  -> high-first, low parent={high}: " << (ok ? "PASS" : "FAIL") << '\n';

		// Execute. The low-prio worker waits on the high-prio future before calling DoTask(),
		// so the two "running" lines appear in fixed order: high first, then low.
		Meta::CScheduler::Execute(sched, view, []
		{
			return true;
		});
	}

	return 0;
}
