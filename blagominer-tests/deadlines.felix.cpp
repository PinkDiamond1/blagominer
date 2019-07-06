#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "reference/BurstMath.h"
#include "hexstring.h"

//+ sample data from felixbrucker: testing if various 'burstmath' modules produce the same results

TEST(RnD_BurstMath_CalcDeadline_Felix, unk523458) {
	auto const height = 523458;
	uint64_t account_nr = 14541460361080926540;
	uint64_t nonce_nr = 804427435;
	uint32_t scoop_nr = 1057;

	uint64_t currentBaseTarget = 74102;
	auto currentSignature = HexString::from("6d0be5b1bf78cab7e37a5573af0074bcc4a885c456d433a2b063f854ba2e422f");

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

	EXPECT_EQ(deadline, 441);
}

TEST(RnD_BurstMath_CalcDeadline_Felix, unk523459) {
	auto const height = 523459;
	uint64_t account_nr = 12389242946914536976;
	uint64_t nonce_nr = 4026905118;
	uint32_t scoop_nr = 3550;

	uint64_t currentBaseTarget = 75275;
	auto currentSignature = HexString::from("7396ad4f201a924215cd21ae227c42d87bbe704fcf04b26d48ec99773ce0dacb");

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

	EXPECT_EQ(deadline, 180);
}

TEST(RnD_BurstMath_CalcDeadline_Felix, unk523460) {
	auto const height = 523460;
	uint64_t account_nr = 5838076873831982979;
	uint64_t nonce_nr = 375474931;
	uint32_t scoop_nr = 1204;

	uint64_t currentBaseTarget = 77583;
	auto currentSignature = HexString::from("18d32dc0c20abba377e544264080e597064485a57ea510e1c9f03bbd00387d54");

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

	EXPECT_EQ(deadline, 536);
}

//- sample data from felixbrucker
