#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "reference/burst/BurstMath.h"
#include "hexstring.h"

TEST(BurstMath_CalculateScoop, Burst000637363) {

	auto const height = 637363;
	auto currentSignature = HexString::from("5b6dbea6f6c25f039a00b15985ffb767e57814f340299435330a5b2bf70db7c6");

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data());

	EXPECT_EQ(scoop_nr, 1278);
}

TEST(BurstMath_CalculateDeadline, GeneratesScoopDataWhenNotProvided) {

	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 16500290252697119892; // 7955621360090688183_13692243649594369992_114440

	auto const height = 637363;
	auto currentSignature = HexString::from("5b6dbea6f6c25f039a00b15985ffb767e57814f340299435330a5b2bf70db7c6");
	uint64_t currentBaseTarget = 45037;
	uint32_t scoop_nr = 1278;

	uint8_t(*scoopLow)[32] = nullptr;
	uint8_t(*scoopHigh)[32] = nullptr;

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data(), scoopLow, scoopHigh);

	std::string scoopLowHex = HexString::string(std::vector<uint8_t>(*scoopLow, *scoopLow + 32));
	std::string scoopHighHex = HexString::string(std::vector<uint8_t>(*scoopHigh, *scoopHigh + 32));

	delete[] scoopLow;
	delete[] scoopHigh;

	EXPECT_EQ("fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c", scoopLowHex);
	EXPECT_EQ("7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9", scoopHighHex);
	EXPECT_EQ(5178030, deadline);
}

TEST(BurstMath_CalculateDeadline, UsesProvidedScoopDataWhenItIsProvided) {

	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 16500290252697119892; // 7955621360090688183_13692243649594369992_114440

	auto const height = 637363;
	auto currentSignature = HexString::from("5b6dbea6f6c25f039a00b15985ffb767e57814f340299435330a5b2bf70db7c6");
	uint64_t currentBaseTarget = 45037;
	uint32_t scoop_nr = 1278;

	{
		auto tmp1 = HexString::from("fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c");
		auto tmp2 = HexString::from("7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9");

		ASSERT_EQ(32, tmp1.size());
		ASSERT_EQ(32, tmp2.size());

		uint8_t(*scoopLow)[32] = (uint8_t(*)[32])(tmp1.data());
		uint8_t(*scoopHigh)[32] = (uint8_t(*)[32])(tmp2.data());

		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data(), scoopLow, scoopHigh);
		EXPECT_EQ(5178030, deadline);
	}

	// below: those two tests below are quite lousy, but if `calcdeadline` ignores one of the input data,
	// the resulting deadline will/should differ, and that's enough for a simple test
	{
		auto tmp1 = HexString::from("8888888888888888888888888888888888888888888888888888888888888888");
		auto tmp2 = HexString::from("7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9");

		ASSERT_EQ(32, tmp1.size());
		ASSERT_EQ(32, tmp2.size());

		uint8_t(*scoopLow)[32] = (uint8_t(*)[32])(tmp1.data());
		uint8_t(*scoopHigh)[32] = (uint8_t(*)[32])(tmp2.data());

		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data(), scoopLow, scoopHigh);
		EXPECT_NE(5178030, deadline);
	}
	{
		auto tmp1 = HexString::from("fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c");
		auto tmp2 = HexString::from("8888888888888888888888888888888888888888888888888888888888888888");

		ASSERT_EQ(32, tmp1.size());
		ASSERT_EQ(32, tmp2.size());

		uint8_t(*scoopLow)[32] = (uint8_t(*)[32])(tmp1.data());
		uint8_t(*scoopHigh)[32] = (uint8_t(*)[32])(tmp2.data());

		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data(), scoopLow, scoopHigh);
		EXPECT_NE(5178030, deadline);
	}
}
