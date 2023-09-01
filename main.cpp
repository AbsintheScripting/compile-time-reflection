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

#include "Foo.meta.h"
#include "Bar.meta.h"
#include "MetaResourceManager.h"

int main()
{
	// Init reflection manager
	constexpr static Meta::CResourceReflectionManager<
		Meta::Bar::CMethod,
		Meta::Foo::CMethodA,
		Meta::Foo::CMethodB,
		Meta::Foo::CMethodC
	> REFLECTION_MANAGER;

	// Retrieve resources
	// type: std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,
	//                  CSomeString<EResourceAccessMode::WRITE>>
	constexpr auto barMethod = REFLECTION_MANAGER.GetResources<Meta::Bar::CMethod>();
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(barMethod).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);
	static_assert( // CSomeString<EResourceAccessMode::WRITE>
		std::get<1>(barMethod).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);
	// type: std::tuple<CSomeNumber<EResourceAccessMode::WRITE>,
	//                  CSomeString<EResourceAccessMode::WRITE,
	//                  CSomeString<EResourceAccessMode::READ,
	//                  CAnotherString<EResourceAccessMode::WRITE>>
	constexpr auto fooMethodC = REFLECTION_MANAGER.GetResources<Meta::Foo::CMethodC>();
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(fooMethodC).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);

	return 0;
}
