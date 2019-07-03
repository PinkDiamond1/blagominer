#include "pch.h"

#include "reference/BurstMath.h"

TEST(BurstMath_CalcDeadline, Deadline_Felix001) {
	auto const height = 523458;
	uint64_t account_nr = 14541460361080926540;
	uint64_t nonce_nr = 804427435;
	uint32_t scoop_nr = 1057;

	uint64_t currentBaseTarget = 74102;
	uint8_t currentSignature[32] = { 0x6d,0x0b,0xe5,0xb1,0xbf,0x78,0xca,0xb7,0xe3,0x7a,0x55,0x73,0xaf,0x00,0x74,0xbc,0xc4,0xa8,0x85,0xc4,0x56,0xd4,0x33,0xa2,0xb0,0x63,0xf8,0x54,0xba,0x2e,0x42,0x2f, };

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature);

	EXPECT_EQ(deadline, 441);
}

TEST(BurstMath_CalcDeadline, Deadline_Felix002) {
	auto const height = 523459;
	uint64_t account_nr = 12389242946914536976;
	uint64_t nonce_nr = 4026905118;
	uint32_t scoop_nr = 3550;

	uint64_t currentBaseTarget = 75275;
	uint8_t currentSignature[32] = { 0x73,0x96,0xad,0x4f,0x20,0x1a,0x92,0x42,0x15,0xcd,0x21,0xae,0x22,0x7c,0x42,0xd8,0x7b,0xbe,0x70,0x4f,0xcf,0x04,0xb2,0x6d,0x48,0xec,0x99,0x77,0x3c,0xe0,0xda,0xcb, };

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature);

	EXPECT_EQ(deadline, 180);
}

TEST(BurstMath_CalcDeadline, Deadline_Felix003) {
	auto const height = 523460;
	uint64_t account_nr = 5838076873831982979;
	uint64_t nonce_nr = 375474931;
	uint32_t scoop_nr = 1204;

	uint64_t currentBaseTarget = 77583;
	uint8_t currentSignature[32] = { 0x18,0xd3,0x2d,0xc0,0xc2,0x0a,0xbb,0xa3,0x77,0xe5,0x44,0x26,0x40,0x80,0xe5,0x97,0x06,0x44,0x85,0xa5,0x7e,0xa5,0x10,0xe1,0xc9,0xf0,0x3b,0xbd,0x00,0x38,0x7d,0x54, };

	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature);

	EXPECT_EQ(deadline, 536);
}

TEST(BurstMath_CalcDeadline, Deadline_QueBt55_BHD001) {
	auto const height = 180169;
	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 13119679609451903322; // 7955621360090688183_13119679609451875184_30512

	uint64_t currentBaseTarget = 11378;
	uint8_t currentSignature[32] = { 0x35,0x17,0xb1,0xbc,0x03,0x12,0x9c,0xd1,0xf6,0xcd,0xc1,0xb7,0x01,0x93,0xc6,0x63,0xdb,0x3c,0x63,0xb7,0xde,0xee,0xf5,0x32,0x42,0x56,0x5e,0xd1,0xcc,0xd6,0x0f,0xf7, };

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature);
	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature);

	EXPECT_EQ(deadline, 17015487);
}

TEST(BurstMath_CalcDeadline, Deadline_QueBt55_Burst001) {
	auto const height = 637359;
	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 13119679609471448000; // 7955621360090688183_13119679609471433376_30512

	uint64_t currentBaseTarget = 46318; // sig == GMI sig, OK
	uint8_t currentSignature[32] = { 0xf5,0x3f,0x4f,0xc1,0xc9,0x59,0xfe,0xe2,0xa5,0x9e,0x6b,0xa0,0xca,0xf8,0xde,0x94,0x53,0x01,0xb6,0x11,0x9e,0x77,0xd8,0x6f,0x05,0x81,0xb4,0x01,0x3f,0x05,0x69,0x35, };

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature);
	// e42097076fef6d78972cd492afba142fb4350d356e93d1059cafab82874eb1f2 267799b0c4d3b398e98925d7907d696d6b3394755536ab0acab2c5dec716e480
	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature);

	EXPECT_EQ(scoop_nr, 1213);
	EXPECT_EQ(deadline, 1142365);
}

TEST(BurstMath_CalcDeadline, Deadline_QueBt55_Burst002) {
	auto const height = 637363;
	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 16500290252697119892; // 7955621360090688183_16500290252697119360_30512

	uint64_t currentBaseTarget = 45037;
	uint8_t currentSignature[32] = { 0x5b,0x6d,0xbe,0xa6,0xf6,0xc2,0x5f,0x03,0x9a,0x00,0xb1,0x59,0x85,0xff,0xb7,0x67,0xe5,0x78,0x14,0xf3,0x40,0x29,0x94,0x35,0x33,0x0a,0x5b,0x2b,0xf7,0x0d,0xb7,0xc6, };

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature);
	// fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c 7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9
	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature);

	EXPECT_EQ(scoop_nr, 1278);
	EXPECT_EQ(deadline, 5178030);
}
