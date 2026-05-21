#pragma once
#include <MetaResource.hpp>

#include "IFooBar.h"

class CBarFoo : public IFooBar
{
public:
	~CBarFoo() override;

	int AbstractMethod() override
	{
		return barFooNum;    // read access CBarFoo::barFooNum
	}

	int VirtualMethod() override
	{
		return fooBarNum;    // read access IFooBar::fooBarNum
	}

private:
	int barFooNum = 0;

public:
	struct _meta
	{
		template <Meta::EResourceAccessMode Mode>
		using TBarFooNum = Meta::CResourceAccess<^^CBarFoo::barFooNum, Mode>;
	};
};
