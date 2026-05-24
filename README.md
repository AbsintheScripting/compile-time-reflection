# Compile-Time Reflection System for Resource Management
[![C++26](https://img.shields.io/badge/dialect-C%2B%2B26-blue)](https://en.cppreference.com/w/cpp/26)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

This project provides a header-only solution for a compile-time reflection system designed for resource management.
The result can be used to create an execution graph for your multi-threaded system,
such as with [entt::flow](https://github.com/skypjack/entt/wiki/Crash-Course:-graph#flow-builder).
A task scheduler example based on this solution and entt::flow is available in the [examples](example/) folder.

## Overview

In a multi-threaded system, a resource collision occurs when multiple threads attempt to write the same resource simultaneously,
or one thread is writing while another is trying to read it.
To solve this problem, you can use mutexes, atomics,
or a more sophisticated lock-free system that orders and schedules tasks to prevent collisions.
This project provides the tools to distinguish between read and write access to resources,
allowing your task scheduler to order and execute tasks safely and efficiently.

## Core Functionality

The core functionality of this project is to provide a mechanism for identifying and managing resource access in a multi-threaded environment.
This involves:
- **Declaring resources of a class:**
    The `Meta.hpp` file provides helper structs for efficiently declaring members and methods for use in your multi-threaded system.
- **Resource Access Modes:**
    When declaring methods, you specify the accessed resources as either read or write mode.
    Later, when declaring accessed resources for your task, you only need to specify the called methods, and the resources will be filtered out.
- **Extracting resource types:**
    The resource-visitor provides an efficient way to extract meta-information about your declared resources, which can then be used by your task scheduler

## Example

Let's say we have two classes, Foo and Bar, used in a multi-threaded task system
where the scheduler orders tasks to prevent resource collision.

CFoo.h:
```cpp
class CFoo
{
	std::string someString;
public:
	int someNumber = 0;

	void Method()
	{
		someNumber = 1;      // write access someNumber
		someString = "Test"; // write access someString
	}
};
```

CBar.h:
```cpp
class CBar
{
public:
	void MethodCallingMethod(CFoo& foo)
	{
		foo.Method();                                  // inherit resources from CFoo::Method
		std::cout << "Foo number: " << foo.someNumber; // read access to public someNumber
	}
}
```

For both classes we declare the meta-information that we use later
when declaring the accessed resources in our tasks.

CFoo.h:
```cpp
class CFoo
{
	std::string someString;
public:
	int someNumber = 0;

	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Resource definitions
		template <TMode Mode>
		using TSomeString = Meta::CResourceAccess<^^CBar::someString, Mode>;
		template <TMode Mode>
		using TSomeNumber = Meta::CResourceAccess<^^CBar::someNumber, Mode>;
		// Method definitions
		using TMethod = Meta::CMethodResources<TSomeNumber<TMode::WRITE>,
		                                       TSomeString<TMode::WRITE>>;
	};

	[[=CMeta::TMethod{}]]
	void Method()
	{
		someNumber = 1;      // write access someNumber
		someString = "Test"; // write access someString
	}
};
```

CBar.h:
```cpp
class CBar
{
public:
	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Method definitions
		using TMethodCallingMethod = Meta::CMethodResources<CFoo::CMeta::TMethod,
		                                                    CFoo::CMeta::TSomeNumber<TMode::READ>>;
	};

	[[=CMeta::TMethodCallingMethod{}]]
	void MethodCallingMethod(CFoo& foo)
	{
		foo.Method();                                  // inherit resources from CFoo::Method
		std::cout << "Foo number: " << foo.someNumber; // read access to public someNumber
	}
}
```

Now with a scheduler, which can consume the meta data annotated on the class methods,
we can build an execution graph (like with the `entt::flow` builder) and enable task execution without collisions.
See the [example](example/) folder for a more detailed and complete example.
If you want to build the example, remember to pull also the entt submodule.

## Annotations
This project uses my open-source [C++ code style](https://gist.github.com/AbsintheScripting/4f2be73c91fc49fc6bc2cefbb2a52895).
