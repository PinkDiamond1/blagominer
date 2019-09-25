#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "inout.h"

TEST(InOutTest_Vector, BuildAFilledString) {
	auto output = make_filled_string(10, L'+');
	EXPECT_THAT(output, L"++++++++++");
}

TEST(InOutTest_Vector, NoCrashWhenNegative) {
	auto output = make_filled_string(-10, L'+');
	EXPECT_THAT(output, L"");
}
