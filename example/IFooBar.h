#pragma once

#include <Meta.hpp>

namespace Meta::FooBar
{
struct MCBarFooVirtualMethod;
struct MCFooBarVirtualMethod;
struct MCBarFooAbstractMethod;
struct MCFooBarAbstractMethod;
}

// Interface for CFooBar and CBarFoo
class IFooBar
{
protected:
	int fooBarNum = 0; // CMeta::TFooBarNum

public:
	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Resource definitions
		template <TMode Mode>
		using TFooBarNum = Meta::CResourceAccess<^^IFooBar::fooBarNum, Mode>;
		// Method definitions
		// Since we don't know which derived class is going to be used at runtime,
		// we include the resource definitions from all derived classes at compile-time
		// and from IFooBar itself.
		// The type list filters will deal with duplicates and read-write pairs.
		using TAbstractMethod = Meta::CMethodResources<
			Meta::FooBar::MCFooBarAbstractMethod,
			Meta::FooBar::MCBarFooAbstractMethod>;
		using TVirtualMethod = Meta::CMethodResources<
			Meta::FooBar::MCFooBarVirtualMethod,
			Meta::FooBar::MCBarFooVirtualMethod,
			TFooBarNum<TMode::WRITE>>;
	};

	virtual ~IFooBar();

	[[=CMeta::TAbstractMethod{}]]
	virtual int AbstractMethod() = 0;
	[[=CMeta::TVirtualMethod{}]]
	virtual int VirtualMethod()
	{
		fooBarNum = 1;    // write access IFooBar::fooBarNum
		return fooBarNum;
	}
};
