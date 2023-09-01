#pragma once
#include "Meta.h"
#include "Bar.h"

namespace Meta::Bar
{
	template <EResourceAccessMode AccessMode>
	struct CSomeNumber : CMemberResourceAccess<CBar, &CBar::someNumber, AccessMode>
	{
	};

	template <EResourceAccessMode AccessMode>
	struct CSomeString : CMemberResourceAccess<CBar, &CBar::someString, AccessMode>
	{
	};

	template <EResourceAccessMode AccessMode>
	struct CAnotherString : CMemberResourceAccess<CBar, &CBar::anotherString, AccessMode>
	{
	};

	struct CMethod : CMethodResources<CSomeNumber<EResourceAccessMode::WRITE>,
	                                  CSomeString<EResourceAccessMode::WRITE>>
	{
	};
}
