#pragma once
#include <string>

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
};
