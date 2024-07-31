#pragma once
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
};
