#include "pch.h"

#include "elevate.h"

TEST(ElevateTest, ProcessParameterQuoting_DoesNothing_WhenNoSpecialChars) {
	std::wstring out;
	ArgvQuote(L"abcDefg", out, false);
	EXPECT_EQ(out, L"abcDefg");
}

TEST(ElevateTest, ProcessParameterQuoting_AddsQuotes_WhenNoSpecialCharsButForceIsEnabled) {
	std::wstring out;
	ArgvQuote(L"abcDefg", out, true);
	EXPECT_EQ(out, L"\"abcDefg\"");
}

TEST(ElevateTest, ProcessParameterQuoting_AddsQuotes_WhenWhitespacesArePresent) {
	std::wstring out;
	ArgvQuote(L"abc efg", out, true);
	EXPECT_EQ(out, L"\"abc efg\"");
}

TEST(ElevateTest, ProcessParameterQuoting_EscapesQuotes_WhenQuotesArePresent) {
	std::wstring out;
	ArgvQuote(L"abc\"efg", out, true);
	EXPECT_EQ(out, L"\"abc\\\"efg\"");
}
