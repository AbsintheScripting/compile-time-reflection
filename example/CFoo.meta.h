#pragma once
#include <Meta.hpp>

#include "CBar.meta.h"
#include "CFoo.h"

namespace Meta::Foo
{
/************
 * Methods
 ************/

struct MMethodA : CFoo::CMeta::TMethodA
{};

struct MMethodB : CFoo::CMeta::TMethodB
{};

struct MMethodC : CFoo::CMeta::TMethodC
{};

struct MReadSomeString : CFoo::CMeta::TReadSomeString
{};
}

namespace Meta
{
// all:
using TFooResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST,
                                             Foo::MMethodA, Foo::MMethodB, Foo::MMethodC, Foo::MReadSomeString>;
#undef GLOBAL_METHOD_RESOURCE_LIST
#define GLOBAL_METHOD_RESOURCE_LIST TFooResourcesList
}
