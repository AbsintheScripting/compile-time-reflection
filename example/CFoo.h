#pragma once
#include <iostream>

#include <Meta.hpp>

#include "CBar.h"

class CFoo
{
	int number = 0; // CMeta::TNumber

public:
	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Resource definitions
		template <TMode Mode>
		using TNumber = Meta::CResourceAccess<^^CFoo::number, Mode>;
		// Method definitions
		using TMethodA = Meta::CMethodResources<TNumber<TMode::WRITE>,
		                                        CBar::CMeta::TPublicWriteSomeNumber,
		                                        CBar::CMeta::TPublicReadSomeString>;
		using TMethodB = Meta::CMethodResources<CBar::CMeta::TMethod,
		                                        CBar::CMeta::TPublicReadSomeString>;
		using TMethodC = Meta::CMethodResources<TMethodB,
		                                        CBar::CMeta::TPublicReadSomeString,
		                                        CBar::CMeta::TSetAnotherString>;
		using TReadSomeString = Meta::CMethodResources<TNumber<TMode::WRITE>,
		                                               CBar::CMeta::TPublicReadSomeString>;
	};

	[[=CMeta::TMethodA{}]]
	void MethodA(CBar& bar)
	{
		number = 1;                                          // write access Foo::number
		bar.someNumber = 0;                                  // write access Bar::someNumber
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
	}

	[[=CMeta::TMethodB{}]]
	void MethodB(CBar& bar)
	{
		bar.Method();                                        // inherit everything from Meta::Bar::Method
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
	}

	[[=CMeta::TMethodC{}]]
	void MethodC(CBar& bar)
	{
		MethodB(bar);                                        // inherit everything from MethodB
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
		bar.SetAnotherString("Test");                      // write access Bar::anotherString
	}

	[[=CMeta::TReadSomeString{}]]
	void ReadSomeString(const CBar& bar)
	{
		std::cout << "Bar string: " + bar.someString + '\n'; // read access Bar::someString
		number = 2;                                          // write access Foo::number
	}
};
