# compile time reflection system
[![C++20](https://img.shields.io/badge/dialect-C%2B%2B20-blue)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

This little project shows how to implement a compile time reflection system for resource management.
The result can then be used for creating an execution graph for your multi-threaded system, 
like with [entt::flow](https://github.com/skypjack/entt/wiki/Crash-Course:-graph#flow-builder) for example.


The output is basically a list of all accessed resources of one routine,
including accessed resources of all sub-routines.
So when you define a task for a multi-threaded system, you only list accessed
resources and called functions without the need to manually go into every
function to check on used resources.
The output is filtered in such a way,
that you only have a list of unique resource types.
Also if you list a resource twice, one as read the other as write, 
only the resource with write access will be listed and the one with read access will be filtered out.

## Example

```cpp

class CBar
{
public:
	void Method()
	{
		someNumber = 1;      // write access someNumber
		someString = "Test"; // write access someString
	}
	void MethodCallingMethod()
	{
		Method();                                  // inherit resources from Method
		std::cout << "Bar string: " << someString; // read access someString
	}
	int someNumber;
	std::string someString;
};

namespace Meta::Bar
{
	// helper struct for CBar::someNumber
	template <EResourceAccessMode AccessMode>
	struct CSomeNumber : CMemberResourceAccess<CBar, &CBar::someNumber, AccessMode>
	{
	};
	// helper struct for CBar::someString
	template <EResourceAccessMode AccessMode>
	struct CSomeString : CMemberResourceAccess<CBar, &CBar::someString, AccessMode>
	{
	};
	// declare used resources for CBar::Method
	struct CMethod : CMethodResources<CSomeNumber<EResourceAccessMode::WRITE>,
	                                  CSomeString<EResourceAccessMode::WRITE>>
	{
	};
	// declare used resources for CBar::MethodCallingMethod
	struct CMethodCallingMethod : CMethodResources<CMethod,
	                                               CSomeString<EResourceAccessMode::READ>>
	{
	};
}

int main()
{
	using TSomeNumberWrite = Meta::Bar::CSomeNumber<Meta::EResourceAccessMode::WRITE>;
	using TSomeStringRead = Meta::Bar::CSomeString<Meta::EResourceAccessMode::READ>;
	using TSomeStringWrite = Meta::Bar::CSomeString<Meta::EResourceAccessMode::WRITE>;
	// Init reflection manager
	constexpr static Meta::CResourceReflectionManager<
		Meta::Bar::CMethod,
		Meta::Bar::CMethodCallingMethod // , ...
	> REFLECTION_MANAGER;
	// Retrieve resources
	constexpr auto barMethod = REFLECTION_MANAGER.GetResources<Meta::Bar::CMethod>();
	// type: const std::tuple<TSomeNumberWrite, TSomeStringWrite>
	static_assert(std::is_same_v<std::decay_t<decltype(barMethod)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite>>);
	static_assert( // CSomeNumber<EResourceAccessMode::WRITE>
		std::get<0>(barMethod).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);
	static_assert( // CSomeString<EResourceAccessMode::WRITE>
		std::get<1>(barMethod).ACCESS_MODE == Meta::EResourceAccessMode::WRITE
	);
	// Retrieve resources recursively
	// (e.g. CMethodResources<CMethod> as CMethod is of type CMethodResources as well)
	// and filter out resources which are listed as read access
	// but also exist as write access in the resources list
	constexpr auto barMethodCallingMethod = REFLECTION_MANAGER.GetResources<Meta::Bar::CMethodCallingMethod>();
	// type: const std::tuple<TSomeNumberWrite, TSomeStringWrite>
	// we filtered out TSomeStringRead because of TSomeStringWrite (write > read)
	static_assert(std::is_same_v<std::decay_t<decltype(barMethodCallingMethod)>,
	                             std::tuple<TSomeNumberWrite,
	                                        TSomeStringWrite>>);
	return 0;
}
```

## Code Style
If you're wondering what code style this is,
it is [my own C++ code style](https://gist.github.com/AbsintheScripting/4f2be73c91fc49fc6bc2cefbb2a52895).
