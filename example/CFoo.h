#pragma once
#include <iostream>

#include "CBar.h"

class CFoo
{
public:
	CFoo()
		: number(0)
	{
	}

	void MethodA(CBar& bar)
	{
		number = 1;                                    // write access Foo::number
		bar.someNumber = 0;                            // write access Bar::someNumber
		std::cout << "Bar string: " << bar.someString; // read access Bar::someString
	}

	void MethodB(CBar& bar)
	{
		bar.Method();                                  // inherit everything from Meta::Bar::Method
		std::cout << "Bar string: " << bar.someString; // read access Bar::someString
	}

	void MethodC(CBar& bar)
	{
		MethodB(bar);                                  // inherit everything from MethodB
		std::cout << "Bar string: " << bar.someString; // read access Bar::someString
		bar.anotherString = "Test";                    // write access Bar::anotherString
	}

private:
	int number;
};
