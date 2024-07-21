#pragma once
#include <Meta.hpp>

#include "CBar.meta.h"
#include "CFoo.h"

namespace Meta::Foo
{
	// private:
	using TNumber = CMember<int, CStringLiteral("number")>;

	// resources:
	template <EResourceAccessMode AccessMode>
	struct CNumber : CMemberResourceAccess<CFoo, TNumber, AccessMode>
	{
	};

	// methods:
	struct CMethodA : CMethodResources<CNumber<EResourceAccessMode::WRITE>,
	                                   Bar::CSomeNumber<EResourceAccessMode::WRITE>,
	                                   Bar::CSomeString<EResourceAccessMode::READ>>
	{
	};

	struct CMethodB : CMethodResources<Bar::CMethod,
	                                   Bar::CSomeString<EResourceAccessMode::READ>>
	{
	};

	struct CMethodC : CMethodResources<CMethodB,
	                                   Bar::CSomeString<EResourceAccessMode::READ>,
	                                   Bar::CAnotherString<EResourceAccessMode::WRITE>>
	{
	};

	struct CReadSomeString : CMethodResources<CNumber<EResourceAccessMode::WRITE>,
	                                          Bar::CSomeString<EResourceAccessMode::READ>>
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
