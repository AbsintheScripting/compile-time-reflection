#pragma once
#include <Meta.hpp>

#include "CBar.meta.h"
#include "CFoo.h"

namespace Meta::Foo
{
	/************
	 * Methods
	 ************/

	struct CMethodA : CMethodResources<CFoo::_meta::TNumber<EResourceAccessMode::WRITE>,
	                                   Bar::CMethod,
	                                   Bar::CPublicReadSomeString>
	{
	};

	struct CMethodB : CMethodResources<Bar::CMethod,
	                                   Bar::CPublicReadSomeString>
	{
	};

	struct CMethodC : CMethodResources<CMethodB,
	                                   Bar::CPublicReadSomeString,
	                                   Bar::CSetAnotherString>
	{
	};

	struct CReadSomeString : CMethodResources<CFoo::_meta::TNumber<EResourceAccessMode::WRITE>,
	                                          Bar::CPublicReadSomeString>
	{
	};
}

namespace Meta
{
	// all:
	using TFooResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST,
	                                             Foo::CMethodA, Foo::CMethodB, Foo::CMethodC, Foo::CReadSomeString>;
	#undef GLOBAL_METHOD_RESOURCE_LIST
	#define GLOBAL_METHOD_RESOURCE_LIST TFooResourcesList
}
