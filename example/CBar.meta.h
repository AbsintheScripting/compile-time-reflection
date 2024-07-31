#pragma once
#include <Meta.hpp>

#include "CBar.h"

namespace Meta::Bar
{
	// public:
	using TSomeNumber = CPublicMember<&CBar::someNumber>;
	using TSomeString = CPublicMember<&CBar::someString>;
	// protected:
	using TAnotherString = CMember<std::string, "anotherString"_sl>;

	// resources:
	template <EResourceAccessMode AccessMode>
	struct CSomeNumber : CMemberResourceAccess<CBar, TSomeNumber, AccessMode>
	{
	};

	template <EResourceAccessMode AccessMode>
	struct CSomeString : CMemberResourceAccess<CBar, TSomeString, AccessMode>
	{
	};

	template <EResourceAccessMode AccessMode>
	struct CAnotherString : CMemberResourceAccess<CBar, TAnotherString, AccessMode>
	{
	};

	// methods:
	struct CPublicReadSomeNumber : CMethodResources<CSomeNumber<EResourceAccessMode::READ>>
	{
	};

	struct CPublicWriteSomeNumber : CMethodResources<CSomeNumber<EResourceAccessMode::WRITE>>
	{
	};

	struct CPublicReadSomeString : CMethodResources<CSomeString<EResourceAccessMode::READ>>
	{
	};

	struct CPublicWriteSomeString : CMethodResources<CSomeString<EResourceAccessMode::WRITE>>
	{
	};

	struct CMethod : CMethodResources<CSomeNumber<EResourceAccessMode::WRITE>,
	                                  CSomeString<EResourceAccessMode::WRITE>>
	{
	};

	struct CSetAnotherString : CMethodResources<CAnotherString<EResourceAccessMode::WRITE>>
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
