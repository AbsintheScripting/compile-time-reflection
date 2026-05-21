#pragma once
#include <string>

#include <MetaResource.hpp>

class CBar
{
public:
	CBar()
		: someNumber(0)
	{
	}

	void Method()
	{
		someNumber = 1;      // write access Bar::someNumber
		someString = "Test"; // write access Bar::someString
	}

	void SetAnotherString(const std::string& value)
	{
		anotherString = value; // write access Bar::anotherString
	}

	int someNumber;
	std::string someString = "Null";

protected:
	std::string anotherString;

public:
	struct _meta
	{
		template <Meta::EResourceAccessMode Mode>
		using TSomeNumber = Meta::CResourceAccess<^^CBar::someNumber, Mode>;
		template <Meta::EResourceAccessMode Mode>
		using TSomeString = Meta::CResourceAccess<^^CBar::someString, Mode>;
		template <Meta::EResourceAccessMode Mode>
		using TAnotherString = Meta::CResourceAccess<^^CBar::anotherString, Mode>;
	};
};
