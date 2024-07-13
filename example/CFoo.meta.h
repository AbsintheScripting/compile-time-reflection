#pragma once
#include "CBar.meta.h"
#include "CFoo.h"
#include "Meta.hpp"

namespace Meta::Foo
{
	using TPrivateNumber = CPrivateField<int, CStringLiteral("number")>;

	template <EResourceAccessMode AccessMode>
	struct CNumber : CPrivateMemberResourceAccess<CFoo, TPrivateNumber, AccessMode>
	{
	};

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
}
