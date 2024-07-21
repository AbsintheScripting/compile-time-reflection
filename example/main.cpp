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
#include <thread>
#include <tuple>
#include <vector>

#include "MetaResourceManager.hpp"
#include "MetaResourceVisitor.hpp"
#include "Task.hpp"

int main()
{
	// check global registered resources
	static_assert(std::tuple_size_v<Meta::CResourceVisitor::TResourceTuple> > 0);
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

	// Init reflection manager
	constexpr static Meta::CResourceReflectionManager<
		Meta::Bar::CMethod,
		Meta::Foo::CMethodA,
		Meta::Foo::CMethodB,
		Meta::Foo::CMethodC,
		Meta::Foo::CReadSomeString
	> REFLECTION_MANAGER;

	// Retrieve resources
	// type: std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,  // TSomeNumberWrite
	//                  CSomeString<EResourceAccessMode::WRITE>>  // TSomeStringWrite
	constexpr auto barMethod = REFLECTION_MANAGER.GetResources<Meta::Bar::CMethod>();
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
	constexpr auto fooMethodC = REFLECTION_MANAGER.GetResources<Meta::Foo::CMethodC>();
	static_assert(std::is_same_v<std::decay_t<decltype(fooMethodC)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite,
	                                        TAnotherStringWrite>>);
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(fooMethodC).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);

	// Runtime test

	const auto myFoo = std::make_unique<CFoo>();
	const auto myBar = std::make_unique<CBar>();

	// Write accesses: Foo::number
	// Read accesses: Bar::someString
	std::function funA = [&]()
	{
		myFoo->ReadSomeString(*myBar);
	};
	// type std::tuple< struct Meta::Bar::CSomeString<0>, struct Meta::Foo::CNumber<1> >
	using TTaskA = CTask<Meta::Foo::CReadSomeString>;
	auto taskA = std::make_unique<TTaskA>(std::move(funA));
	// Write accesses: Bar::someNumber and Bar::someString
	std::function funB = [&]()
	{
		myBar->Method();
	};
	// type std::tuple< struct Meta::Bar::CSomeString<1>, struct Meta::Bar::CSomeNumber<1> >
	using TTaskB = CTask<Meta::Bar::CMethod>;
	auto taskB = std::make_unique<TTaskB>(std::move(funB));
	// Write accesses: Bar::anotherString
	std::function funC = [&]()
	{
		myBar->SetAnotherString("Test");
	};
	// type std::tuple< struct Meta::Bar::CAnotherString<1> >
	using TTaskC = CTask<Meta::Bar::CSetAnotherString>;
	auto taskC = std::make_unique<TTaskC>(std::move(funC));

	// Conflicts: taskA and taskB, because funA wants to read Bar::someString while funB tries to write it
	std::vector<std::unique_ptr<ITask>> tasks;
	tasks.push_back(std::move(taskA));
	tasks.push_back(std::move(taskB));
	tasks.push_back(std::move(taskC));
	std::vector<size_t> finishedThreads;

	// Get the global resource list as tuple
	constexpr auto seqResourceTuple = std::make_index_sequence<std::tuple_size_v<
		Meta::CResourceVisitor::TResourceTuple>>{};
	auto printTuple = [&]<typename Tuple, std::size_t... Is>(std::index_sequence<Is...>, Tuple tuple)
	{
		(
			(std::cout << typeid(std::tuple_element_t<Is, Tuple>).name() <<
				" (" << typeid(std::tuple_element_t<Is, Tuple>).hash_code() << ")" << std::endl
			),
			...
		);
	};
	std::cout << "Meta::TResourceTuple types:" << std::endl;
	printTuple(seqResourceTuple, Meta::CResourceVisitor::TResourceTuple{});

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
	std::cout << "" << std::endl;

	std::cout << "Checking task types:" << std::endl;
	for (auto&& task : tasks)
	{
		std::any resources = task->GetResources();
		std::type_index typeInfo = resources.type();
		std::cout << "To check: \t" << typeInfo.name() << " (" << typeInfo.hash_code() << ")" << std::endl;
		Meta::CResourceVisitor::VisitAny(
			resources,
			[&]<typename T>(std::tuple<T> resource_tuple)
			{
				if constexpr (Meta::method_resources<T>)
				{
					std::type_index taskType = typeid(T);
					auto resourcesTuple = T::GetResources();
					std::type_index taskResourceTuple = typeid(resourcesTuple);
					std::cout << "Found type: \t\t"
						<< taskType.name() << " (" << taskType.hash_code() << ")" << std::endl;
					std::cout << "Resources: \t\t"
						<< taskResourceTuple.name() << " (" << taskResourceTuple.hash_code() << ")" << std::endl;
				}
			}
		);
	}

	return 0;
}
