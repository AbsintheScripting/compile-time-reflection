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
#include <MetaResourceVisitor.hpp>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "CTaskScheduler.h"
#include "MetaResourceList.h"
#include "Task.hpp"

// Test structures to test the concepts forward_declared_type and complete_type
class CIncomplete; // Forward declaration

class CComplete
{
	int data = 0;
};

int main()
{
	/***************
	 * Static tests
	 ***************/
	static_assert(Meta::forward_declared_type<CIncomplete>, "CIncomplete should be an incomplete type");
	static_assert(Meta::complete_type<CComplete>, "CComplete should be a complete type");
	// check string literals
	using Meta::operator ""_sl;
	static_assert(std::same_as<decltype(Meta::CStringLiteral("same")), decltype("same"_sl)>);
	static_assert(Meta::CStringLiteral("same") == "same"_sl);
	static_assert(Meta::CStringLiteral("not_same") != "NotSame"_sl);
	// check global registered resources
	static_assert(std::tuple_size_v<Meta::TGlobalResourceList> > 0);
	// check members
	static_assert(std::is_same_v<Meta::Foo::TNumber::TMemberType, int>);
	static_assert(Meta::Foo::TNumber::MEMBER_NAME == Meta::CStringLiteral("number"));
	static_assert(std::is_same_v<Meta::Bar::TSomeNumber::TMemberType, int>);
	static_assert(std::is_same_v<Meta::Bar::TSomeString::TMemberType, std::string>);
	static_assert(std::is_same_v<Meta::Bar::TAnotherString::TMemberType, std::string>);
	static_assert(Meta::Bar::TAnotherString::MEMBER_NAME == Meta::CStringLiteral("anotherString"));

	// define aliases to check
	using TSomeNumberWrite = Meta::Bar::CSomeNumber<Meta::EResourceAccessMode::WRITE>;
	using TSomeStringRead = Meta::Bar::CSomeString<Meta::EResourceAccessMode::READ>;
	using TSomeStringWrite = Meta::Bar::CSomeString<Meta::EResourceAccessMode::WRITE>;
	using TAnotherStringWrite = Meta::Bar::CAnotherString<Meta::EResourceAccessMode::WRITE>;
	using TSomeMethodResources = Meta::CMethodResources<TSomeNumberWrite, TSomeStringWrite>;

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
	// check CMethodResources::TTypes (like TUniqueTypes but as tuple and in inverse order)
	static_assert(std::is_same_v<Meta::Bar::CMethod::TTypes,
	                             std::tuple<TSomeStringWrite,
	                                        TSomeNumberWrite>>);
	static_assert(std::is_same_v<Meta::Foo::CMethodB::TTypes,
	                             std::tuple<TSomeStringRead,
	                                        Meta::Bar::CMethod>>);
	static_assert(std::is_same_v<Meta::Foo::CMethodC::TTypes,
	                             std::tuple<TAnotherStringWrite,
	                                        TSomeStringRead,
	                                        Meta::Foo::CMethodB>>);
	// check recursion (by mixing a CMethodResources' param pack with a CMethodResources and a CMemberResourceAccess)
	using TRecursiveMethodResources = Meta::CMethodResources<TSomeMethodResources, TSomeStringRead>;
	static_assert(std::is_same_v<decltype(TRecursiveMethodResources::GetResources()),
	                             std::tuple<TSomeNumberWrite, TSomeStringWrite, TSomeStringRead>>);
	static_assert(std::is_same_v<decltype(TRecursiveMethodResources::GetFilteredResources()),
	                             std::tuple<TSomeNumberWrite, TSomeStringWrite>>); // filters out TSomeStringRead

	// Retrieve filtered resources
	// type: std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,  // TSomeNumberWrite
	//                  CSomeString<EResourceAccessMode::WRITE>>  // TSomeStringWrite
	constexpr auto barMethod = Meta::Bar::CMethod::GetFilteredResources();
	static_assert(std::is_same_v<std::decay_t<decltype(barMethod)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite>>);
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(barMethod).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);
	static_assert( // CSomeString<EResourceAccessMode::WRITE>
		std::get<1>(barMethod).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
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
	constexpr auto fooMethodC = Meta::Foo::CMethodC::GetFilteredResources();
	static_assert(std::is_same_v<std::decay_t<decltype(fooMethodC)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite,
	                                        TAnotherStringWrite>>);
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(fooMethodC).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);

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
	using TTaskA = CTask<Meta::Foo::CReadSomeString>;
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
	using TTaskB = CTask<Meta::Bar::CMethod>;
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
	using TTaskC = CTask<Meta::Bar::CSetAnotherString>;
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
	using TTaskD = CTask<Meta::CNoResources>;
	auto taskD = std::make_shared<TTaskD>(std::move(funD));

	// Task E
	// Write accesses: none
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
	// type CMethodA std::tuple<struct Meta::Foo::CNumber<1>,struct Meta::Bar::CSomeNumber<1>,struct Meta::Bar::CSomeString<0> >
	// type CMethodB std::tuple<struct Meta::Bar::CSomeNumber<1>,struct Meta::Bar::CSomeString<1> >
	// type CMethodC std::tuple<struct Meta::Bar::CSomeNumber<1>,struct Meta::Bar::CSomeString<1>,struct Meta::Bar::CAnotherString<1> >

	// type filtered std::tuple<struct Meta::Foo::CNumber<1>,struct Meta::Bar::CSomeNumber<1>,struct Meta::Bar::CSomeString<1>,struct Meta::Bar::CAnotherString<1> >
	using TTaskE = CTask<Meta::Foo::CMethodA, Meta::Foo::CMethodB, Meta::Foo::CMethodC>;
	auto taskE = std::make_shared<TTaskE>(std::move(funE));

	// Add tasks to our scheduler queue and task list
	// Conflicts: taskA and taskB, because funA wants to read Bar::someString while funB tries to write it
	std::queue<std::shared_ptr<ITask>> schedulerTaskQueue;
	schedulerTaskQueue.push(taskA);
	schedulerTaskQueue.push(taskB);
	schedulerTaskQueue.push(taskC);
	schedulerTaskQueue.push(taskD);
	schedulerTaskQueue.push(taskE);
	std::vector<std::shared_ptr<ITask>> tasks;
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
			std::cout << "\t" << (idx + 1) << ". check:\t" << typeInfo.name() << " (" << typeInfo.hash_code() << ")" << std::endl;
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

	// Schedule tasks and execute them
	// Task A and B execution shall not overlap, since they have a conflicting resource
	// Task E has conflicts with A, B and C
	// Task D has no conflicts and can run in parallel with all tasks
	std::cout << "" << std::endl;
	std::cout << "Executing tasks:" << std::endl;
	CTaskScheduler taskScheduler{};
	taskScheduler.OrderAndExecuteTasks(schedulerTaskQueue);

	return 0;
}
