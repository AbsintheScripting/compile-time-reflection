#pragma once
#include <Meta.hpp>

// include interfaces and all derived classes
#include "IFooBar.h"
#include "CFooBar.h"
#include "CBarFoo.h"

namespace Meta::FooBar
{
/************
 * Methods
 ************/

// IFooBar
struct MIFooBarAbstractMethod : IFooBar::CMeta::TAbstractMethod
{};

struct MIFooBarVirtualMethod : IFooBar::CMeta::TVirtualMethod
{};

// CFooBar
struct MCFooBarAbstractMethod : CFooBar::CMeta::TAbstractMethod
{};

struct MCFooBarVirtualMethod : CFooBar::CMeta::TVirtualMethod
{};

// CBarFoo
struct MCBarFooAbstractMethod : CBarFoo::CMeta::TAbstractMethod
{};

struct MCBarFooVirtualMethod : CBarFoo::CMeta::TVirtualMethod
{};

}

namespace Meta
{
// all:
using TFooBarResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST,
                                                FooBar::MIFooBarAbstractMethod,
                                                FooBar::MIFooBarVirtualMethod,
                                                FooBar::MCFooBarAbstractMethod,
                                                FooBar::MCFooBarVirtualMethod,
                                                FooBar::MCBarFooAbstractMethod,
                                                FooBar::MCBarFooVirtualMethod>;
#undef GLOBAL_METHOD_RESOURCE_LIST
#define GLOBAL_METHOD_RESOURCE_LIST TFooBarResourcesList
}
