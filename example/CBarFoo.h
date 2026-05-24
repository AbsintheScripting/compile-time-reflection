#pragma once
#include <Meta.hpp>

#include "IFooBar.h"

class CBarFoo : public IFooBar
{
	int barFooNum = 0; // CMeta::TBarFooNum
public:
	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Resource definitions
		template <TMode Mode>
		using TBarFooNum = Meta::CResourceAccess<^^CBarFoo::barFooNum, Mode>;
		// Method definitions
		using TAbstractMethod = Meta::CMethodResources<TBarFooNum<TMode::READ>>;
		using TVirtualMethod = Meta::CMethodResources<IFooBar::CMeta::TFooBarNum<TMode::READ>>;
	};

	~CBarFoo() override;

	[[=CMeta::TAbstractMethod{}]]
	int AbstractMethod() override
	{
		return barFooNum;    // read access CBarFoo::barFooNum
	}

	[[=CMeta::TVirtualMethod{}]]
	int VirtualMethod() override
	{
		return fooBarNum;    // read access IFooBar::fooBarNum
	}
};
