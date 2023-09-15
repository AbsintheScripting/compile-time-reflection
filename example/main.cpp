/**
 * The idea is to construct a simple reflection tool to reflect on
 * resource usage and how they are accessed (read or write mode).
 * It is basically a list of all accessed resources of one routine,
 * including accessed resources of all sub-routines.
 * So when I define a task for a multi-threaded system, I only list accessed
 * resources and called functions without the need to manually go into every
 * function to check on used resources.
 */

#include <tuple>

#include "CFoo.meta.h"
#include "CBar.meta.h"
#include "MetaResourceManager.hpp"

int main()
{
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
		Meta::Foo::CMethodC
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

	return 0;
}
