#pragma once
#include <Meta.hpp>

#include "CBar.h"

namespace Meta::Bar
{
/************
 * Methods
 ************/

struct MPublicReadSomeNumber : CBar::CMeta::TPublicReadSomeNumber
{};

struct MPublicWriteSomeNumber : CBar::CMeta::TPublicWriteSomeNumber
{};

struct MPublicReadSomeString : CBar::CMeta::TPublicReadSomeString
{};

struct MPublicWriteSomeString : CBar::CMeta::TPublicWriteSomeString
{};

struct MMethod : CBar::CMeta::TMethod
{};

struct MSetAnotherString : CBar::CMeta::TSetAnotherString
{};
}

namespace Meta
{
// all:
using TBarResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST,
                                             Bar::MPublicReadSomeNumber, Bar::MPublicWriteSomeNumber,
                                             Bar::MPublicReadSomeString, Bar::MPublicWriteSomeString,
                                             Bar::MMethod, Bar::MSetAnotherString>;
#undef GLOBAL_METHOD_RESOURCE_LIST
#define GLOBAL_METHOD_RESOURCE_LIST TBarResourcesList
}
