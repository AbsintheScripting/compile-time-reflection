#pragma once
#include <MetaResource.hpp>

#include "IFooBar.h"

class CFooBar : public IFooBar
{
public:
	~CFooBar() override;

	int AbstractMethod() override
	{
		return fooBarNum;    // read access IFooBar::fooBarNum
	}

	int VirtualMethod() override
	{
		otherFooBarNum = 1;  // write access CFooBar::otherFooBarNum
		return fooBarNum;    // read access IFooBar::fooBarNum
	}

private:
	int otherFooBarNum = 0;

public:
	struct _meta
	{
		template <Meta::EResourceAccessMode Mode>
		using TOtherFooBarNum = Meta::CResourceAccess<^^CFooBar::otherFooBarNum, Mode>;
	};
};
