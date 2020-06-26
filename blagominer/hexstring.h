#pragma once
#include "common-pragmas.h"

#include <array>
#include <memory>
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

	static void hex2bin(std::string const& src, uint8_t* target)
	{
		char const * source = src.c_str();

		if (src.length() % 2 == 1)
			*(target++) = char2int(*(source++));

		while (source[0] && source[1])
		{
			*(target++) = char2int(source[0]) * 16 + char2int(source[1]);
			source += 2;
		}
	}

	static char nibble2char(uint8_t nibble)
	{
		if (nibble >= 0x0 && nibble <= 0x9) return '0' + nibble;
		if (nibble >= 0xA && nibble <= 0xF) return 'a' + nibble - 0xA;
		throw std::invalid_argument("Invalid input string");
	}

	static uint8_t char2int(wchar_t input)
	{
		if (input >= L'0' && input <= L'9') return input - L'0';
		if (input >= L'A' && input <= L'F') return input - L'A' + 10;
		if (input >= L'a' && input <= L'f') return input - L'a' + 10;
		throw std::invalid_argument("Invalid input string");
	}

	static void hex2bin(std::wstring const& src, uint8_t* target)
	{
		wchar_t const* source = src.c_str();

		if (src.length() % 2 == 1)
			*(target++) = char2int(*(source++));

		while (source[0] && source[1])
		{
			*(target++) = char2int(source[0]) * 16 + char2int(source[1]);
			source += 2;
		}
	}

	static wchar_t nibble2wchar(uint8_t nibble)
	{
		if (nibble >= 0x0 && nibble <= 0x9) return L'0' + nibble;
		if (nibble >= 0xA && nibble <= 0xF) return L'a' + nibble - 0xA;
		throw std::invalid_argument("Invalid input string");
	}

public:

	/**
	 * Parses a string of hexadecimal digits into an array of bytes.
	 * Assumes natural order of bits in byte: 0x01 is 1, 0x10 is 16.
	 * Assumes natural order of bytes in string: AABB is [0xAA,0xBB].
	 *
	 * @param text the text to parse
	 * @return vector of 8bit bytes
	 * @throws std::invalid_argument when input contains non-hexdigit characters
	 */
	static std::vector<uint8_t> from(std::string const& text)
	{
		std::vector<uint8_t> tmp;
		tmp.resize((text.size() + 1) / 2);
		hex2bin(text, tmp.data());
		return tmp;
	}
	static std::vector<uint8_t> from(std::wstring const& text)
	{
		std::vector<uint8_t> tmp;
		tmp.resize((text.size() + 1) / 2);
		hex2bin(text, tmp.data());
		return tmp;
	}

	template<size_t N>
	static std::unique_ptr<std::array<uint8_t, N>> arrayfrom(std::string const& text)
	{
		auto vector = from(text);
		if (vector.size() != N) throw std::invalid_argument("Wrong input data size");
		std::unique_ptr<std::array<uint8_t,N>> tmp = std::make_unique<std::array<uint8_t, N>>();
		std::copy(vector.begin(), vector.end(), tmp->begin());
		return tmp;
	}

	template<size_t N>
	static std::unique_ptr<std::array<uint8_t, N>> arrayfrom(std::wstring const& text)
	{
		auto vector = from(text);
		if (vector.size() != N) throw std::invalid_argument("Wrong input data size");
		std::unique_ptr<std::array<uint8_t, N>> tmp = std::make_unique<std::array<uint8_t, N>>();
		std::copy(vector.begin(), vector.end(), tmp->begin());
		return tmp;
	}

	template<size_t N>
	static std::string string(std::array<uint8_t, N> const& data)
	{
		std::string tmp;
		tmp.reserve(data.size() * 2);
		for (auto it = data.begin(); it < data.end(); ++it)
		{
			tmp.push_back(nibble2char(*it / 0x10));
			tmp.push_back(nibble2char(*it % 0x10));
		}
		return tmp;
	}

	template<size_t N>
	static std::wstring wstring(std::array<uint8_t, N> const& data)
	{
		std::wstring tmp;
		tmp.reserve(data.size() * 2);
		for (auto it = data.begin(); it < data.end(); ++it)
		{
			tmp.push_back(nibble2wchar(*it / 0x10));
			tmp.push_back(nibble2wchar(*it % 0x10));
		}
		return tmp;
	}

	static std::string string(std::vector<uint8_t> const& data)
	{
		std::string tmp;
		tmp.reserve(data.size() * 2);
		for (auto it = data.begin(); it < data.end(); ++it)
		{
			tmp.push_back(nibble2char(*it / 0x10));
			tmp.push_back(nibble2char(*it % 0x10));
		}
		return tmp;
	}

	static std::wstring wstring(std::vector<uint8_t> const& data)
	{
		std::wstring tmp;
		tmp.reserve(data.size() * 2);
		for (auto it = data.begin(); it < data.end(); ++it)
		{
			tmp.push_back(nibble2wchar(*it / 0x10));
			tmp.push_back(nibble2wchar(*it % 0x10));
		}
		return tmp;
	}
};
