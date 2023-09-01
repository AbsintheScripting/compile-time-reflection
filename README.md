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

## Example

```cpp
class CBar
{
public:
	void Method()
	{
		someNumber = 1;      // write access Bar::someNumber
		someString = "Test"; // write access Bar::someString
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
}

int main()
{
	// Init reflection manager
	constexpr static Meta::CResourceReflectionManager<
		Meta::Bar::CMethod
		// ...
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
	return 0;
}
```

### TODO
Filter out resources which are listed as read access but also exist as write access in the list.
