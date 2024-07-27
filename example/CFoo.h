#pragma once
#include <iostream>

#include "CBar.h"

class CFoo
{
public:
	void MethodA(CBar& bar)
	{
		number = 1;                                          // write access Foo::number
		bar.someNumber = 0;                                  // write access Bar::someNumber
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
	}

	void MethodB(CBar& bar)
	{
		bar.Method();                                        // inherit everything from Meta::Bar::Method
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
	}

	void MethodC(CBar& bar)
	{
		MethodB(bar);                                        // inherit everything from MethodB
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
		bar.SetAnotherString("Test");                        // write access Bar::anotherString
	}

	void ReadSomeString(const CBar& bar)
	{
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
		number = 2;                                          // write access Foo::number
	}
private:
	int number = 0;
};
