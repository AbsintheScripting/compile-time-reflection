#pragma once
#include <Meta.hpp>

#include "CBar.h"

namespace Meta::Bar
{
	/************
	 * Methods
	 ************/

	struct CPublicReadSomeNumber : CMethodResources<CBar::_meta::TSomeNumber<EResourceAccessMode::READ>>
	{
	};

	struct CPublicWriteSomeNumber : CMethodResources<CBar::_meta::TSomeNumber<EResourceAccessMode::WRITE>>
	{
	};

	struct CPublicReadSomeString : CMethodResources<CBar::_meta::TSomeString<EResourceAccessMode::READ>>
	{
	};

	struct CPublicWriteSomeString : CMethodResources<CBar::_meta::TSomeString<EResourceAccessMode::WRITE>>
	{
	};

	struct CMethod : CMethodResources<CBar::_meta::TSomeNumber<EResourceAccessMode::WRITE>,
	                                  CBar::_meta::TSomeString<EResourceAccessMode::WRITE>>
	{
	};

	struct CSetAnotherString : CMethodResources<CBar::_meta::TAnotherString<EResourceAccessMode::WRITE>>
	{
	};
}

namespace Meta
{
	// all:
	using TBarResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST,
	                                             Bar::CPublicReadSomeNumber, Bar::CPublicWriteSomeNumber,
	                                             Bar::CPublicReadSomeString, Bar::CPublicWriteSomeString,
	                                             Bar::CMethod, Bar::CSetAnotherString>;
	#undef GLOBAL_METHOD_RESOURCE_LIST
	#define GLOBAL_METHOD_RESOURCE_LIST TBarResourcesList
}
