#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "reference/BurstMath.h"
#include "hexstring.h"

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst641159WithGenSigFromDump) {
	// based on data from LIVE debugging session
	// 23:20:17 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Wed, 10 Jul 2019 21:20:17 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":34819,"generationSignature":"ed377f7ea5f857c565baa9e44aa94f5c8162c2fdf430d6eccdac4b856c6b5b68","height":641159,"targetDeadline":31536000}
	/*
		HEIGHT: [00007FF76D13EF60]      87 C8 09 00 = latest
		SIG:    [00007FF76D13F020]      = totally different
		00007FF76D13F020  BC EA A3 39 BD 7C D1 B1 33 74 9B F3 9F 3E E6 51
		00007FF76D13F030  8C C9 4E A3 16 FF 7C 49 CD 66 F6 D2 39 A3 9E F5
		BAS:    [00007FF76D13F150]      03 88 00 00 ... (34819) = latest
		NON:    rcx                     E0 0E CB 01 18 77 12 B6 ]+1  (res1 was the best)
		ACC:    ....                    6E680B79EFB72AB7 = ok
		SCOOP:  [00007FF76D13F014]      49 03 00 00 = noidea if it's synced or not
		  => DL     000015DE5AA6ECBA
	*/

	auto const height = 0x09C887; // 641159
	//auto currentSignature = HexString::from("ed377f7ea5f857c565baa9e44aa94f5c8162c2fdf430d6eccdac4b856c6b5b68"); // gensig from the pool
	auto currentSignature = HexString::from("BCEAA339BD7CD1B133749BF39F3EE6518CC94EA316FF7C49CD66F6D239A39EF5"); // 'corrected' gensig used by the miner
	uint64_t currentBaseTarget = 0x00008803; // 34819
	uint64_t account_nr = 7955621360090688183;

	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 1;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0x000015DE5AA6ECBA/*+000000000000604D*/); // 7B F6 B7 BD C3 61 9E 0B 33 74 9B F3 9F 3E E6 51
	}
	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 0;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0xD2FD1CE8ED9F53FE / currentBaseTarget); // FE 53 9F ED E8 1C FD D2 00 00 00 00 00 00 00 00
	}
	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 2;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0x2C39F0C1A3A30488 / currentBaseTarget); // 88 04 A3 A3 C1 F0 39 2C 80 02 00 00 00 00 00 00
	}
	{
		uint64_t nonce_nr = 0xB612771801CB0EE0 + 3;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 0x0349
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 0x3280915CBDEC5885 / currentBaseTarget); // 85 58 EC BD 5C 91 80 32 00 00 00 00 00 CC 1D 00
	}
}

TEST(RnD_BurstMath_LoadExternal_QueLap, dcminerSimplyUnpacked) {

}
