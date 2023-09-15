#pragma once
#include <string>

class CBar
{
public:
	void Method()
	{
		someNumber = 1;      // write access Bar::someNumber
		someString = "Test"; // write access Bar::someString
	}

	int someNumber;
	std::string someString;
	std::string anotherString;
};
