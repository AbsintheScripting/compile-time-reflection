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

	// derived first
	// use the name of the class + name of the method

	// CFooBar
	struct CFooBarAbstractMethod : CMethodResources<IFooBar::_meta::TFooBarNum<EResourceAccessMode::READ>>
	{
	};

	struct CFooBarVirtualMethod : CMethodResources<CFooBar::_meta::TOtherFooBarNum<EResourceAccessMode::WRITE>,
	                                               IFooBar::_meta::TFooBarNum<EResourceAccessMode::READ>>
	{
	};

	// CBarFoo
	struct CBarFooAbstractMethod : CMethodResources<CBarFoo::_meta::TBarFooNum<EResourceAccessMode::READ>>
	{
	};

	struct CBarFooVirtualMethod : CMethodResources<IFooBar::_meta::TFooBarNum<EResourceAccessMode::READ>>
	{
	};

	// IFooBar
	// Since we don't know which derived class is going to be used at runtime,
	// we include the resource definitions from all derived classes at compile-time
	// and from IFooBar itself.
	// The type list filters will deal with duplicates and read-write pairs.

	struct IFooBarAbstractMethod : CMethodResources<CFooBarAbstractMethod, CBarFooAbstractMethod>
	{
	};

	struct IFooBarVirtualMethod : CMethodResources<CFooBarVirtualMethod,
	                                               CBarFooVirtualMethod,
	                                               IFooBar::_meta::TFooBarNum<EResourceAccessMode::WRITE>>
	{
	};
}

namespace Meta
{
	// all:
	using TFooBarResourcesList = TRegisterResources<GLOBAL_METHOD_RESOURCE_LIST,
	                                                FooBar::IFooBarAbstractMethod,
	                                                FooBar::IFooBarVirtualMethod,
	                                                FooBar::CFooBarAbstractMethod,
	                                                FooBar::CFooBarVirtualMethod,
	                                                FooBar::CBarFooAbstractMethod,
	                                                FooBar::CBarFooVirtualMethod>;
#undef GLOBAL_METHOD_RESOURCE_LIST
#define GLOBAL_METHOD_RESOURCE_LIST TFooBarResourcesList
}
