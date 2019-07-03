#pragma once

#include <string>
#include <vector>

class HexString
{
private:
	static uint8_t char2int(char input)
	{
		if (input >= '0' && input <= '9') return input - '0';
		if (input >= 'A' && input <= 'F') return input - 'A' + 10;
		if (input >= 'a' && input <= 'f') return input - 'a' + 10;
		throw std::invalid_argument("Invalid input string");
	}

	static void hex2bin(const char* src, uint8_t* target)
	{
		while (src[0] && src[1])
		{
			*(target++) = char2int(src[0]) * 16 + char2int(src[1]);
			src += 2;
		}
	}

public:

	static std::vector<uint8_t> from(std::string const& text)
	{
		std::vector<uint8_t> tmp;
		tmp.resize(text.size() / 2);
		hex2bin(text.c_str(), tmp.data());
		return tmp;
	}
};
