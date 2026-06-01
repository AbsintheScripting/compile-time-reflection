# Compile-Time Reflection System for Resource Management
[![C++26](https://img.shields.io/badge/dialect-C%2B%2B26-blue)](https://en.cppreference.com/w/cpp/26)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

This project provides a header-only solution for a compile-time reflection system designed for resource management,
together with a matching task scheduler that uses the reflected information to order parallel work without conflicts.
Both the resource description (`Meta.hpp`) and the scheduler (`MetaScheduler.hpp`)
are pure C++26 with no third-party dependencies.
A complete example combining everything is available in the [examples](example/) folder.

## Overview

In a multi-threaded system, a resource collision occurs
when multiple threads attempt to write the same resource simultaneously,
or one thread is writing while another is trying to read it.
To solve this problem, you can use mutexes, atomics,
or a more sophisticated lock-free system that orders and schedules tasks to prevent collisions.
This project provides the tools to distinguish between read and write access to resources,
and a scheduler that turns that information into a parent-child dependency schedule at runtime,
allowing your tasks to execute safely and as parallel as their resource sets allow.

## Core Functionality

The core functionality of this project is to provide a mechanism for identifying
and managing resource access in a multi-threaded environment.
This involves:
- **Declaring resources of a class:**
    The `Meta.hpp` file provides helper structs for efficiently declaring members
    and methods for use in your multi-threaded system.
- **Resource Access Modes:**
    When declaring methods, you specify the accessed resources as either read or write mode.
    Later, when declaring accessed resources for your task, you only need to specify the called methods,
    and the resources will be filtered out.
- **Compile-time conflict matrix:**
    Once methods are registered into a global list (`GLOBAL_METHOD_RESOURCE_LIST`),
    `Meta::CSchedulerTraits` precomputes the full NxN method-vs-method conflict matrix
    as a `constexpr std::array<std::bitset<N>, N>`, so every conflict relation is resolved at compile-time.
- **Priority-aware dependency scheduling:**
    `Meta::CScheduler` consumes the precomputed matrix and turns a per-tick task queue into a list of tasks
    with explicit predecessor indices, in O(n² / 64) bitset operations.
    Priority controls ordering between tasks but never introduces artificial barriers between non-conflicting tasks.

## Example

Let's say we have two classes, `CFoo` and `CBar`, used in a multi-threaded task system
where the scheduler orders tasks to prevent resource collision.

`CFoo.h`:
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

`CBar.h`:
```cpp
class CBar
{
public:
    void MethodCallingMethod(CFoo& foo)
    {
        foo.Method();                                  // inherit resources from CFoo::Method
        std::cout << "Foo number: " << foo.someNumber; // read access to public someNumber
    }
};
```

### 1. Declaring resources

For each class we declare the meta-information that we use later when declaring the accessed resources in our tasks.
Resource templates spell out read/write mode; method aliases compose them and (optionally) other method aliases.

`CFoo.h`:
```cpp
class CFoo
{
    std::string someString; // CMeta::TSomeString
public:
    int someNumber = 0; // CMeta::TSomeNumber

    struct CMeta
    {
        using TMode = Meta::EResourceAccessMode;
        // Resource definitions
        template <TMode Mode>
        using TSomeString = Meta::CResourceAccess<^^CFoo::someString, Mode>;
        template <TMode Mode>
        using TSomeNumber = Meta::CResourceAccess<^^CFoo::someNumber, Mode>;
        // Method definitions
        using TMethod = Meta::CMethodResources<TSomeNumber<TMode::WRITE>,
                                               TSomeString<TMode::WRITE>>;
    };

    [[=CMeta::TMethod{}]] // Annotated Meta::CMethodResource
    void Method()
    {
        someNumber = 1;      // write access someNumber
        someString = "Test"; // write access someString
    }
};
```

`CBar.h`:
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

    [[=CMeta::TMethodCallingMethod{}]] // Annotated Meta::CMethodResource
    void MethodCallingMethod(CFoo& foo)
    {
        foo.Method();                                  // inherit resources from CFoo::Method
        std::cout << "Foo number: " << foo.someNumber; // read access to public someNumber
    }
};
```

### 2. Registering methods in the global registry

Each class header gets a companion `*.meta.h` that wraps every annotated method in a named struct
and appends them to the global method registry.
The scheduler walks this registry at compile time to build the conflict matrix.
And it is also useful to resolve circular dependencies between headers.

`CFoo.meta.h`:
```cpp
#pragma once
#include <Meta.hpp>
#include "CFoo.h"

namespace Meta::Foo
{
    struct MMethod : CFoo::CMeta::TMethod {};
}

namespace Meta
{
    using TFooResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST, Foo::MMethod>;
    #undef  GLOBAL_METHOD_RESOURCE_LIST
    #define GLOBAL_METHOD_RESOURCE_LIST TFooResourcesList
}
```

A small `MetaResourceList.h` then includes every `*.meta.h` so that after that include the macro
`GLOBAL_METHOD_RESOURCE_LIST` resolves to the complete list of registered methods.

### 3. Defining tasks

Wrap a callable in `CTask<...>`, listing the method-resource annotations the callable will exercise.
The annotations decide the task's bitset masks (`METHOD_MASK`, `CONFLICT_MASK`),
which are filled at compile time from the global conflict matrix.
Tasks can take an optional priority, the default is `Meta::EPriority::Lowest`.

```cpp
#include "MetaResourceList.h"
#include "MetaTask.hpp"

// Task A: high priority, will read+write everything Foo::Method touches.
auto taskA = std::make_shared<CTask<Meta::Foo::MMethod>>(
    [&]{ myFoo.Method(); },
    Meta::EPriority::High
);

// Task B: default priority, reads CFoo::someNumber and calls CFoo::Method through CBar.
auto taskB = std::make_shared<CTask<Meta::Bar::MMethodCallingMethod>>(
    [&]{ myBar.MethodCallingMethod(myFoo); }
);
```

Priority is a plain `int32_t` axis (`Meta::TPriority`);
the `Meta::EPriority` struct exposes named anchors (`Lowest`, `Low`, `Normal`, `High`, `Critical`),
and callers are free to add their own constants in between without modifying the scheduler.

### 4. Running the scheduler

Feed a span of task handles into `Meta::CScheduler::Schedule` to obtain a `Meta::TSchedule`,
a vector where each entry names a task and the indices of the previously-scheduled entries it must wait for.
Pass the same span and the schedule into `Meta::CScheduler::Execute` to run everything with `std::async`.
Workers block on their parent futures and `Execute` blocks until the whole tick is done,
so a normal game loop just calls it once per tick.

```cpp
#include <MetaScheduler.hpp>

std::vector<std::shared_ptr<Meta::ITask>> tasks = { taskA, taskB };
std::span<const std::shared_ptr<Meta::ITask>> view{tasks};

// Phase 1: build the dependency schedule (O(n² / 64) bitset ANDs).
const Meta::TSchedule schedule = Meta::CScheduler::Schedule(view);

// Phase 2: execute. The last argument is a `should-continue?` predicate
// that the workers re-check before invoking the task body, useful for graceful shutdown.
Meta::CScheduler::Execute(schedule, view, []{ return true; });
```

For one-shot use, `Meta::CScheduler::OrderAndExecute(view)` performs both phases in a single call.

The scheduler is templated on the handle type (`scheduler_task_ptr` concept),
so anything with `GetPriority()`, `GetMethodMask()`, `GetConflictMask()` and `DoTask()` works,
like a raw `Meta::ITask*`, `std::shared_ptr<Meta::ITask>`,
or your own packet wrapper that forwards those calls to an embedded task.

See the [example](example/) folder for a fuller, multi-task scenario including priority demonstrations.

## Annotations
This project uses my open-source [C++ code style](https://gist.github.com/AbsintheScripting/4f2be73c91fc49fc6bc2cefbb2a52895).
