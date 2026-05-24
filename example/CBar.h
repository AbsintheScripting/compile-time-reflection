#pragma once
#include <Meta.hpp>
#include <string>

class CBar
{
protected:
	std::string anotherString; // CMeta::TAnotherString
public:
	int someNumber; // CMeta::TSomeNumber
	std::string someString = "Null"; // CMeta::TSomeString

	struct CMeta
	{
		using TMode = Meta::EResourceAccessMode;
		// Resource definitions
		template <TMode Mode>
		using TAnotherString = Meta::CResourceAccess<^^CBar::anotherString, Mode>;
		template <TMode Mode>
		using TSomeNumber = Meta::CResourceAccess<^^CBar::someNumber, Mode>;
		template <TMode Mode>
		using TSomeString = Meta::CResourceAccess<^^CBar::someString, Mode>;
		// Member definitions
		using TPublicReadSomeNumber = Meta::CMethodResources<TSomeNumber<TMode::READ>>;
		using TPublicWriteSomeNumber = Meta::CMethodResources<TSomeNumber<TMode::WRITE>>;
		using TPublicReadSomeString = Meta::CMethodResources<TSomeString<TMode::READ>>;
		using TPublicWriteSomeString = Meta::CMethodResources<TSomeString<TMode::WRITE>>;
		// Method definitions
		using TMethod = Meta::CMethodResources<TSomeNumber<TMode::WRITE>,
		                                       TSomeString<TMode::WRITE>>;
		using TSetAnotherString = Meta::CMethodResources<TAnotherString<TMode::WRITE>>;
	};

	CBar()
		: someNumber(0)
	{}

	[[=CMeta::TMethod{}]] // Annotate resource usage
	void Method()
	{
		someNumber = 1;      // write access Bar::someNumber
		someString = "Test"; // write access Bar::someString
	}

	[[=CMeta::TSetAnotherString{}]] // Annotate resource usage
	void SetAnotherString(const std::string& value)
	{
		anotherString = value; // write access Bar::anotherString
	}
};
