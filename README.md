# Compile-Time Reflection System for Resource Management
[![C++20](https://img.shields.io/badge/dialect-C%2B%2B20-blue)](https://en.cppreference.com/w/cpp/20)
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
- Declaring resources of a class:
    The `Meta.hpp` file provides helper structs for efficiently declaring members and methods for use in your multi-threaded system.
- Resource Access Modes:
    When declaring methods, you specify the accessed resources as either read or write mode.
    Later, when declaring accessed resources for your task, you only need to specify the called methods, and the resources will be filtered out.
- Extracting resource types:
    The resource-visitor provides an efficient way to extract meta-information about your declared resources, which can then be used by your task scheduler

## Example

Let's say we have two classes, Foo and Bar, used in a multi-threaded task system
where the scheduler orders tasks to prevent resource collision.

CFoo.h:
```cpp
class CFoo
{
public:
	void Method()
	{
		someNumber = 1;      // write access someNumber
		someString = "Test"; // write access someString
	}

	int someNumber = 0;
private:
	std::string someString;
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
		std::cout << "Foo string: " << foo.someString; // read access someString
	}
}
```

For both classes we declare the meta-information that we use later
when declaring the accessed resources in our tasks.

CFoo.meta.h:
```cpp
namespace Meta::Foo
{
	// public:
	using TSomeNumber = CPublicMember<&CFoo::someNumber>;
	// private:
	using TSomeString = CMember<std::string, CStringLiteral("someString")>;

	// resources:
	template <EResourceAccessMode AccessMode>
	struct CSomeNumber : CMemberResourceAccess<CFoo, TSomeNumber, AccessMode> {};
	template <EResourceAccessMode AccessMode>
	struct CSomeString : CMemberResourceAccess<CFoo, TSomeString, AccessMode> {};

	// methods:
	struct CMethod : CMethodResources<CSomeNumber<EResourceAccessMode::WRITE>,
	                                  CSomeString<EResourceAccessMode::WRITE>> {};
	struct CPublicReadSomeNumber : CMethodResources<CSomeNumber<EResourceAccessMode::READ>>{};
	struct CPublicWriteSomeNumber : CMethodResources<CSomeNumber<EResourceAccessMode::WRITE>>{};
}

namespace Meta
{
	// all:
	using TFooResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST, Foo::CMethod,
	                                             Foo::CPublicReadSomeNumber, Foo::CPublicWriteSomeNumber>;
	#undef GLOBAL_METHOD_RESOURCE_LIST
	#define GLOBAL_METHOD_RESOURCE_LIST TFooResourcesList
}
```

CBar.meta.h:
```cpp
namespace Meta::Bar
{
	// methods:
	struct CMethodCallingMethod : CMethodResources<Foo::CMethod,
	                                               Foo::CSomeString<EResourceAccessMode::READ>> {};
}

namespace Meta
{
	// all:
	using TBarResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST, Bar::CMethodCallingMethod>;
	#undef GLOBAL_METHOD_RESOURCE_LIST
	#define GLOBAL_METHOD_RESOURCE_LIST TBarResourcesList
}
```

With the trick of undefining and redefining the macro `GLOBAL_METHOD_RESOURCE_LIST`,
we can gather all resources in a neat type list used by the resource-visitor for lookups.
We just need to include all meta headers in one resource list header and alias it.

MetaResourceList.h:
```cpp
// include all meta resource headers here
#include "CBar.meta.h"
#include "CFoo.meta.h"

namespace Meta
{
	using TGlobalResourceList = GLOBAL_METHOD_RESOURCE_LIST::TMethodResources;
}
```

Let's define our tasks and declare used resources.
Then move the task queue into the scheduler to order and start the tasks.

main.cpp:
```cpp
int main()
{
	// Create our test objects
	const auto myFoo = std::make_unique<CFoo>();
	const auto myBar = std::make_unique<CBar>();

	// Create some tasks

	// Task A
	// Write accesses: Foo::someNumber, Foo::someString
	// Read accesses: none
	std::function funA = [&]()
	{
		std::cout << "Execute function A\n";
		myFoo->Method();
		std::cout << "Function A end\n";
	};
	// declare used methods and resources
	using TTaskA = CTask<Meta::Foo::CMethod>;
	auto taskA = std::make_shared<TTaskA>(std::move(funA));

	// Task B
	// Write accesses: Foo::someNumber, Foo::someString
	// Read accesses: Foo::someString
	std::function funB = [&]()
	{
		std::cout << "Execute function B\n";
		myBar->MethodCallingMethod(*myFoo);
		std::cout << "Function B end\n";
	};
	// declare used methods and resources
	using TTaskB = CTask<Meta::Bar::CMethod>;
	auto taskB = std::make_shared<TTaskB>(std::move(funB));

	// Add tasks to our scheduler queue
	// Conflicts: taskA and taskB, because funA wants to write Foo::someNumber and Foo::someString while funB tries the same
	std::queue<std::shared_ptr<ITask>> schedulerTaskQueue;
	schedulerTaskQueue.push(taskA);
	schedulerTaskQueue.push(taskB);

	// Schedule tasks and execute them
	// Task A and B execution shall not overlap, since they have a conflicting resource
	std::cout << "Executing tasks:" << std::endl;
	CTaskScheduler taskScheduler{};
	taskScheduler.OrderAndExecuteTasks(schedulerTaskQueue);

	return 0;
}
```

Let's have a look inside the task scheduler and see the resource-visitor in action:
```cpp
using TResourceVisitor = Meta::CResourceVisitor<Meta::TGlobalResourceList>;

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
			  ? builder.rw(tuple_args.GetHashCode()) // declares the resource as read-write access
			  : builder.ro(tuple_args.GetHashCode())) // declares the resource as read-only access
			, ...);
	};

	while (!task_queue.empty())
	{
		taskList.push_back(std::move(task_queue.front()));
		task_queue.pop();

		auto taskId = reinterpret_cast<entt::id_type>(
			static_cast<void*>(taskList.back().get()) // <- use pointer as uid
		);
		builder.bind(taskId); // registers the task

		// since we get our resources only as std::any,
		// we need to use the visitor pattern to retrieve the original std::tuple
		std::any taskResources = taskList.back()->GetResources();
		// The VisitAny method takes our std::any and a lambda to call with it, if it finds the resource
		TResourceVisitor::VisitAny(
			taskResources,
			[&]<typename T>(std::tuple<T> resources_tuple)
			{
				if constexpr (Meta::method_resources<T>)
				{
					// we receive a filtered resource list as std::tuple,
					// if we have declared for example Foo::someString as read and as write access,
					// we only get it as write access, because that is higher prioritized,
					// since only task is allowed to do this operation at a time
					constexpr auto filteredResources = T::GetFilteredResources();
					// calls our lambda for all filtered resources that registers the resources for this task
					std::apply(registerResources, filteredResources);
				}
			}
		);
	}

[...]
```

Then we build our execution graph based on the entt::flow builder,
enabling task execution without collisions.
See the [example](example/) folder for a more detailed and complete example.
If you want to build the example, remember to pull the entt submodule.

## Annotations
This project uses my open-source [C++ code style](https://gist.github.com/AbsintheScripting/4f2be73c91fc49fc6bc2cefbb2a52895).
