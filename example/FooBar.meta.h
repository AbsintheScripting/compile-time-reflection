#pragma once
#include <Meta.hpp>

// include interfaces and all derived classes
#include "IFooBar.h"
#include "CFooBar.h"
#include "CBarFoo.h"

namespace Meta::FooBar
{
	/************
	 * Members
	 ************/

	// IFooBar
	// protected:
	using TFooBarNum = CMember<int, "fooBarNum"_sl>;

	// CFooBar
	// private:
	using TOtherFooBarNum = CMember<int, "otherFooBarNum"_sl>;

	// CBarFoo
	// private:
	using TBarFooNum = CMember<int, "barFooNum"_sl>;

	/************
	 * Resources
	 ************/

	// IFooBar
	template <EResourceAccessMode AccessMode>
	struct CFooBarNum : CMemberResourceAccess<IFooBar, TFooBarNum, AccessMode>
	{
	};

	// CFooBar
	template <EResourceAccessMode AccessMode>
	struct COtherFooBarNum : CMemberResourceAccess<CFooBar, TOtherFooBarNum, AccessMode>
	{
	};

	// CBarFoo
	template <EResourceAccessMode AccessMode>
	struct CBarFooNum : CMemberResourceAccess<CBarFoo, TBarFooNum, AccessMode>
	{
	};

	/************
	 * Methods
	 ************/

	// derived first
	// use the name of the class + name of the method

	// CFooBar
	struct CFooBarAbstractMethod : CMethodResources<CFooBarNum<EResourceAccessMode::READ>>
	{
	};

	struct CFooBarVirtualMethod : CMethodResources<COtherFooBarNum<EResourceAccessMode::WRITE>,
	                                               CFooBarNum<EResourceAccessMode::READ>>
	{
	};

	// CBarFoo
	struct CBarFooAbstractMethod : CMethodResources<CBarFooNum<EResourceAccessMode::READ>>
	{
	};

	struct CBarFooVirtualMethod : CMethodResources<CFooBarNum<EResourceAccessMode::READ>>
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
	                                               CFooBarNum<EResourceAccessMode::WRITE>>
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
