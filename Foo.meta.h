#pragma once
#include "Meta.h"
#include "Bar.meta.h"

namespace Meta::Foo
{
	struct CMethodA : CMethodResources<Bar::CSomeNumber<EResourceAccessMode::WRITE>,
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
}
