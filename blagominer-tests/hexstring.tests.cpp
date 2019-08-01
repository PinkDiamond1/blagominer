#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "hexstring.h"

using ::testing::ElementsAre;

TEST(HexStringTest_Vector, ParsesLowercaseStrings) {
	auto const input = "ab12cd34e56f";
	auto output = HexString::from(input);
	EXPECT_THAT(output, ElementsAre(0xab, 0x12, 0xcd, 0x34, 0xe5, 0x6f));
}

TEST(HexStringTest_Vector, ParsesUppercaseStrings) {
	auto const input = "AB12CD34E56F";
	auto output = HexString::from(input);
	EXPECT_THAT(output, ElementsAre(0xab, 0x12, 0xcd, 0x34, 0xe5, 0x6f));
}

TEST(HexStringTest_Vector, ParsesMixedcaseStrings) {
	auto const input = "aB12Cd34e56F";
	auto output = HexString::from(input);
	EXPECT_THAT(output, ElementsAre(0xab, 0x12, 0xcd, 0x34, 0xe5, 0x6f));
}

TEST(HexStringTest_Vector, ParsesNonEvenStrings) {
	auto const input = "b6dbea6f6c25f039a00";
	auto output = HexString::from(input);
	EXPECT_THAT(output, ElementsAre(0x0b, 0x6d, 0xbe, 0xa6, 0xf6, 0xc2, 0x5f, 0x03, 0x9a, 0x00));
}

TEST(HexStringTest_Vector, StringifiesHexDataForEachByte) {
	auto const input = std::vector<uint8_t>({ 0x1b, 0x6d, 0xbe, 0xa6, 0xf6, 0xc2, 0x5f, 0x03, 0x9a, 0x00 });
	auto output = HexString::string(input);
	EXPECT_THAT(output, "1b6dbea6f6c25f039a00");
}

TEST(HexStringTest_Vector, StringifiesHexDataWithLeadingZeroes) {
	auto const input = std::vector<uint8_t>({ 0x00, 0x0d, 0xbe, 0xa6, 0xf6, 0xc2, 0x5f, 0x03, 0x9a, 0x00 });
	auto output = HexString::string(input);
	EXPECT_THAT(output, "000dbea6f6c25f039a00");
}

TEST(HexStringTest_Array, ThrowsWhenInputSizeDoesntMatch) {
	auto const input = "b6dbea6f6c25f039a00";

	// this tests _that_ the expected exception is thrown
	EXPECT_THROW({
		try
		{
			HexString::arrayfrom<16>(input);
		}
		catch (const std::invalid_argument& e)
		{
			// EXPECT_...
			throw;
		}
	}, std::invalid_argument);
}

TEST(HexStringTest_Array, ParsesNonEvenStrings) {
	auto const input = "b6dbea6f6c25f039a00";
	auto output = HexString::arrayfrom<10>(input);
	EXPECT_EQ(10, output->size());
	EXPECT_THAT(*output, ElementsAre(0x0b, 0x6d, 0xbe, 0xa6, 0xf6, 0xc2, 0x5f, 0x03, 0x9a, 0x00));
}

TEST(HexStringTest_Array, StringifiesHexDataForEachByte) {
	auto const input = std::array<uint8_t, 10>({ 0x1b, 0x6d, 0xbe, 0xa6, 0xf6, 0xc2, 0x5f, 0x03, 0x9a, 0x00 });
	auto output = HexString::string(input);
	EXPECT_THAT(output, "1b6dbea6f6c25f039a00");
}
