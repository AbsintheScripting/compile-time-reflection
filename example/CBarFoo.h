#pragma once
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
};
