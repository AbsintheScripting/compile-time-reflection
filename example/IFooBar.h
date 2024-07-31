#pragma once

// Interface for CFooBar and CBarFoo
class IFooBar
{
public:
	virtual ~IFooBar();

	virtual int AbstractMethod() = 0;
	virtual int VirtualMethod()
	{
		fooBarNum = 1;    // write access IFooBar::fooBarNum
		return fooBarNum;
	}

protected:
	int fooBarNum = 0;
};
