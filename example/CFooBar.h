#pragma once
#include <Meta.hpp>

#include "IFooBar.h"

class CFooBar : public IFooBar
{
	int otherFooBarNum = 0; // CMeta::TOtherFooBarNum

public:
	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Resource definitions
		template <TMode Mode>
		using TOtherFooBarNum = Meta::CResourceAccess<^^CFooBar::otherFooBarNum, Mode>;
		// Method definitions
		using TAbstractMethod = Meta::CMethodResources<IFooBar::CMeta::TFooBarNum<TMode::READ>>;
		using TVirtualMethod = Meta::CMethodResources<TOtherFooBarNum<TMode::WRITE>,
		                                                    IFooBar::CMeta::TFooBarNum<TMode::READ>>;
	};

	~CFooBar() override;

	[[=CMeta::TAbstractMethod{}]]
	int AbstractMethod() override
	{
		return fooBarNum;    // read access IFooBar::fooBarNum
	}

	[[=CMeta::TVirtualMethod{}]]
	int VirtualMethod() override
	{
		otherFooBarNum = 1;  // write access CFooBar::otherFooBarNum
		return fooBarNum;    // read access IFooBar::fooBarNum
	}
};
