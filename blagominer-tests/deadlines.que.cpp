#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "reference/burst/BurstMath.h"
#include "reference/diskcoin/DiskcoinMath.h"
#include "hexstring.h"

//+ test sample data from my own miners: some random failed DLs I took to inspect

TEST(Diag_BurstMath_CalcDeadline_QueLap, Boom000000633) {
	// Related log excerpts:
	// 10:04:01.739 2.300001.0_AVX2
	// ............................
	// 23:01:54.017 *! GMI Boom: Received new mining info: {"height":633,"baseTarget":104268,"generationSignature":"9e2d1570a477296ccbfdaf689cd7674635749ffaa4a7ee90424e3d83c2e2a7a4","targetDeadline":31536000}
	// 23:01:56.079 found deadline = 20131418 nonce = 13692243649594423672 for account : 7955621360090688183 file : 7955621360090688183_13692243649594369992_114440
	// 23:01:56.563 Boom: {"error":{"message":"DL verification failed, your plot file might be corrupt. AccountId: 7955621360090688183 | Nonce: 13692243649594423672","code":10}}
	// ...
	// 23:01:56.786 *! GMI Boom: Received new mining info: {"height":634,"baseTarget":100476,"generationSignature":"9171514aaa2d7a1584ec00aa019e60841f2b914b3a13d723d193b1c65b50ee6c","targetDeadline":31536000}

	// Test idea:

	// Actually, none. This case is trivial and evident from the logfile. See analysis below.
	// Just doing typical DL check so the test does "something".

	auto const height = 633;
	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 13692243649594423672; // 7955621360090688183_13692243649594369992_114440

	uint64_t currentBaseTarget = 104268;
	auto currentSignature = HexString::from("9e2d1570a477296ccbfdaf689cd7674635749ffaa4a7ee90424e3d83c2e2a7a4");

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data());
	// fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c 7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9
	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

	//?EXPECT_EQ(scoop_nr, -);
	EXPECT_EQ(deadline, 20131418);

	// Analysis:

	// The miner completely correctly calculated DL:20131418 for block 633
	// It even sent it to the server, and had that DL rejected.
	// Short 200ms later the UpdaterThread noticed that the block height has changed and new round started.
	// This a classic showcase for a race between 'updater' and 'worker': updater checks height data periodically,
	// so it almost always happens that the round changed in-between the polls.
	// A possible fix would be to get active notification from the pool.
	// Getting new/current block height info in the rejection message would be nice as well.
}

TEST(Diag_BurstMath_CalcDeadline_QueLap, BHD000180168) {
	// Related log excerpts:
	// 17:51:19.201 2.300001.0_SSE
	// ...................................................................
	// 20:52:42.222 *! GMI BHD: Received new mining info: HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: 151\r\nDate: Thu, 27 Jun 2019 18:52:42 GMT\r\nConnection: close\r\n\r\n{"height":180168,"baseTarget":11406,"generationSignature":"594328a4d33d46ade89d1f44a04b94f4d2d8dd5dc3b82ca4f45cdc8378c55048","targetDeadline":31536000}
	// 20:52:42.236 Interrupting current mining progress. New BHD block.
	// 20:52:42.236 Waiting for worker threads to shut down.
	// 20:52:42.236 All worker threads shut down.
	// 20:52:42.236 New block, no mining has been interrupted.
	// 20:52:42.236 Coin queue: BHD (180168).
	// 20:52:42.236 ------------------------    New BHD block: 180168
	// 20:52:42.236 [# 180168|BHD       |Start   ] Base Target   11406 � Net Diff  1606627 TiB � PoC2
	// .........
	// 20:52:57.180 *! GMI BHD: Received new mining info: HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: 151\r\nDate: Thu, 27 Jun 2019 18:52:57 GMT\r\nConnection: close\r\n\r\n{"height":180169,"baseTarget":11378,"generationSignature":"3517b1bc03129cd1f6cdc1b70193c663db3c63b7deeef53242565ed1ccd60ff7","targetDeadline":31536000}
	// ..
	// 20:52:57.197 Interrupting current mining progress. New BHD block.
	// 20:52:57.197 Waiting for worker threads to shut down.
	// ..................
	// 20:53:17.070 found deadline=17015487 nonce=13119679609451903322 for account: 7955621360090688183 file: 7955621360090688183_13119679609451875184_30512
	// 20:53:17.070 [ 7955621360090688183|BHD       |Worker] DL found     :    17015487
	// 20:53:17.098 [ 7955621360090688183|BHD       |Sender] DL sent      :    17015487     196d 22:31:27
	// ...
	// 20:53:17.287 BHD: {"error":{"message":"DL verification failed, your plot file might be corrupt. AccountId: 7955621360090688183 | Nonce: 13119679609451903322","code":10}}
	// .....
	// 20:53:19.590 [0] Finished directory @f:\burst-data.
	// 20:53:19.591 All worker threads shut down.
	// 20:53:19.591 New block, no mining has been interrupted.
	// 20:53:19.591 Coin queue: BHD (180169).
	// 20:53:19.591 ------------------------    New BHD block: 180169
	// 20:53:19.591 [# 180169|BHD       |Start   ] Base Target   11378 � Net Diff  1610581 TiB � PoC2

	// Test idea:

	// Test checks which gensig the 17015487 came from.
	// Basically, if this test passes, that means that version 2.300001.0_SSE has/had problems with interrupting worker threads. See analysis below.

	{
		auto const height = 180169;
		uint64_t account_nr = 7955621360090688183;
		uint64_t nonce_nr = 13119679609451903322; // 7955621360090688183_13119679609451875184_30512

		uint64_t currentBaseTarget = 11378;
		auto currentSignature = HexString::from("3517b1bc03129cd1f6cdc1b70193c663db3c63b7deeef53242565ed1ccd60ff7");

		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data());
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

		// below: check that the pool was right and that this DL does not match metadata of block 180169
		EXPECT_NE(deadline, 17015487);
	}
	{
		auto const height = 180168;
		uint64_t account_nr = 7955621360090688183;
		uint64_t nonce_nr = 13119679609451903322; // 7955621360090688183_13119679609451875184_30512

		uint64_t currentBaseTarget = 11406;
		auto currentSignature = HexString::from("594328a4d33d46ade89d1f44a04b94f4d2d8dd5dc3b82ca4f45cdc8378c55048");

		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data());
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

		// below: check that the miner really found that DL according to the metadata of block 180168
		EXPECT_EQ(deadline, 17015487);
	}

	// Analysis:

	// DL was rejected. Miner found 17015487, but the pool didn't agree.
	// Analysis of the log below shows that the miner received a new GMI:180169,
	// which should cause the miner to pause/abort currently mined coins,
	// but the miner apparently kept mining at old height for prolonged time and the DL was from GMI:180168
	// Note the '20:53:19.591 New block, no mining has been interrupted.' - either it's message bug, or all worker threads simply finished.
	// In either way, "aborting threads" took over 22 seconds, that's hardly acceptable.
}

TEST(Diag_BurstMath_CalcDeadline_QueLap, Burst000637359) {
	// Related log excerpts:
	// 08:11:50.909 2.300001.0_SSE
	// ...........................
	// 08:13:50.595 *! GMI Burst: Received new mining info: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Sun, 30 Jun 2019 06:13:52 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":46318,"generationSignature":"f53f4fc1c959fee2a59e6ba0caf8de945301b6119e77d86f0581b4013f056935","height":637359,"targetDeadline":31536000}
	// ...........................
	// 08:14:37.384 *! GMI Burst: Received new mining info: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Sun, 30 Jun 2019 06:14:39 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":42813,"generationSignature":"276576207506deb93df7d198d060474e7250a9c194a732272ec4d48f95e3a3c9","height":637360,"targetDeadline":31536000}
	// ..
	// 08:14:37.407 Interrupting current mining progress. New Burst block.
	// 08:14:37.407 Waiting for worker threads to shut down.
	// ...........
	// 08:14:44.358 SSEF: found deadline=1142365 scoop=1213 nonce=13119679609471448000 for account: 7955621360090688183 file: 7955621360090688183_13119679609471433376_30512
	// 08:14:44.358 SIG: 1142365 <= f53f4fc1c959fee2a59e6ba0caf8de945301b6119e77d86f0581b4013f056935
	// 08:14:44.358 SCP: 1142365 <= e42097076fef6d78972cd492afba142fb4350d356e93d1059cafab82874eb1f2 267799b0c4d3b398e98925d7907d696d6b3394755536ab0acab2c5dec716e480
	// 08:14:44.358 [ 7955621360090688183|Burst     |Worker] DL found     :     1142365
	// ...
	// 08:14:44.612 [ 7955621360090688183|Burst     |Sender] DL sent      :     1142365      13d 05:19:25
	// ....
	// 08:14:45.143 Confirmer Burst: Deadline 1142365 sent with error: {"errorCode":"1008","errorDescription":"deadline exceeds deadline limit of the pool"}
	// 08:14:45.144 increaseConflictingDeadline Burst
	// 08:14:45.144 Confirmer Burst: Deadline should have been accepted (1142365 <= 13080869). Fast block or corrupted file?
	// 08:14:45.144 ----Fast block or corrupted file?----
	// Burst sent deadline:	1142365
	// Target deadline:	13080869 
	// ----
	// 08:14:45.144 [ERROR 1008] Burst: deadline exceeds deadline limit of the pool
	// .............
	// 08:15:01.689 [0] Finished directory @f:\burst-data.
	// 08:15:01.689 All worker threads shut down.
	// 08:15:01.689 New block, no mining has been interrupted.
	// 08:15:01.690 Coin queue: Burst (637360), Boom (760).
	// 08:15:01.690 ------------------------    New Burst block: 637360
	// 08:15:01.690 [# 637360|Burst     |Start   ] Base Target   42813 � Net Diff   428028 TiB � PoC2

	// Test idea:

	// Test checks which gensig the 1142365 came from.
	// Basically, if this test passes, that means that version 2.300001.0_SSE has/had problems with interrupting worker threads. See analysis below.

	auto const height = 637359;
	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 13119679609471448000; // 7955621360090688183_13119679609471433376_30512

	uint64_t currentBaseTarget = 46318; // sig == GMI sig, OK
	auto currentSignature = HexString::from("f53f4fc1c959fee2a59e6ba0caf8de945301b6119e77d86f0581b4013f056935");

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data());
	// e42097076fef6d78972cd492afba142fb4350d356e93d1059cafab82874eb1f2 267799b0c4d3b398e98925d7907d696d6b3394755536ab0acab2c5dec716e480
	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

	EXPECT_EQ(scoop_nr, 1213);
	EXPECT_EQ(deadline, 1142365);

	// Analysis:

	// DL was rejected. Miner found 1142365, but the pool didn't agree.
	// Analysis of the log below shows that the miner received a new GMI:637360,
	// which should cause the miner to pause/abort currently mined coins,
	// but the miner apparently kept mining at old height for prolonged time and the DL was from GMI:637359
	// Note the '08:15:01.689 New block, no mining has been interrupted.' - either it's message bug, or all worker threads simply finished.
	// In either way, "aborting threads" took over 24 seconds, that's hardly acceptable.
}

TEST(Diag_BurstMath_CalcDeadline_QueLap, Burst000637363) {
	// Related log excerpts:
	// 08:11:50.909 2.300001.0_SSE
	// ...........................
	// 08:24:40.352 *! GMI Burst: Received new mining info: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Sun, 30 Jun 2019 06:24:42 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":45037,"generationSignature":"5b6dbea6f6c25f039a00b15985ffb767e57814f340299435330a5b2bf70db7c6","height":637363,"targetDeadline":31536000}
	// ...........................
	// 08:24:59.683 *! GMI Burst: Received new mining info: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Sun, 30 Jun 2019 06:25:01 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":44860,"generationSignature":"9199e9add27e2edd69894256183ff5c5f37390c467757c525995fbb5e8b417a8","height":637364,"targetDeadline":31536000}
	// ...........................
	// 08:25:04.822 SSEF: found deadline=5178030 scoop=1278 nonce=16500290252697119892 for account: 7955621360090688183 file: 7955621360090688183_16500290252697119360_30512
	// 08:25:04.822 SIG: 5178030 <= 5b6dbea6f6c25f039a00b15985ffb767e57814f340299435330a5b2bf70db7c6
	// 08:25:04.822 SCP: 5178030 <= fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c 7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9
	// 08:25:04.822 [ 7955621360090688183|Burst     |Worker] DL found     :     5178030
	// ......
	// 08:25:05.163 [ 7955621360090688183|Burst     |Sender] DL sent      :     5178030      59d 22:20:30
	// ......
	// 08:25:05.643 Confirmer Burst: Deadline 5178030 sent with error: {"errorCode":"1008","errorDescription":"deadline exceeds deadline limit of the pool"}
	// ...........................
	// 08:25:17.839 [1] Finished directory @g:\burst-data.
	// 08:25:17.968 New block, no mining has been interrupted.
	// 08:25:17.968 Coin queue: Burst (637364).
	// 08:25:17.968 ------------------------    New Burst block: 637364
	// 08:25:17.968 [# 637364|Burst     |Start   ] Base Target   44860 � Net Diff   408497 TiB � PoC2

	// Test idea:

	// Test checks which gensig the 5178030 came from.
	// Basically, if this test passes, that means that version 2.300001.0_SSE has/had problems with interrupting worker threads. See analysis below.

	auto const height = 637363;
	uint64_t account_nr = 7955621360090688183;
	uint64_t nonce_nr = 16500290252697119892; // 7955621360090688183_16500290252697119360_30512

	uint64_t currentBaseTarget = 45037;
	auto currentSignature = HexString::from("5b6dbea6f6c25f039a00b15985ffb767e57814f340299435330a5b2bf70db7c6");

	uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data());
	// fd0b525fcf7bee41d33a4c5b4dae3092b9957b7921497d33fd851be75669219c 7d18b3b4dd886a0e65422564f52af27d9f216f1ec0f8f0bdfb347ee0445a96a9
	unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());

	EXPECT_EQ(scoop_nr, 1278);
	EXPECT_EQ(deadline, 5178030);

	// Analysis:

	// DL was rejected. Miner found 5178030, but the pool didn't agree.
	// Analysis of the log below shows that the miner received a new GMI:637364,
	// which should cause the miner to pause/abort currently mined coins,
	// but the miner apparently kept mining at old height for prolonged time and the DL was from GMI:637363
	// Note the '08:25:17.968 New block, no mining has been interrupted.' - either it's message bug, or all worker threads simply finished.
	// In either way, "aborting threads" took over 18.5 seconds, that's hardly acceptable.
}

//- test sample data from my own miners

//+ sample data obtainer from running dcminer on burst pool: voiplanparty.com

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst640788differsFromClassicBurst) {
	// 640788
	// 22:57:36 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Tue, 09 Jul 2019 20:57:37 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":62772,"generationSignature":"26e31a80a2302e5bf8ecc132d8c6af761341e3f332afd886022cec162047b356","height":640788,"targetDeadline":31536000}
	// 22:57:40 found deadline=5816864 nonce=13692243649592120299 for account: 7955621360090688183 file: 7955621360090688183_13692243649592081192_114440

	auto const height = 640788;
	auto currentSignature = HexString::from("26e31a80a2302e5bf8ecc132d8c6af761341e3f332afd886022cec162047b356");
	uint64_t currentBaseTarget = 62772;
	uint64_t account_nr = 7955621360090688183;

	{
		uint64_t nonce_nr = 13692243649592120299;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 24
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/5816864); // burst:132860195681833
	}
}

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst640789differsFromClassicBurst) {
	// 640789
	// 22:58:46 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Tue, 09 Jul 2019 20:58:46 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":63956,"generationSignature":"cfff797ad97df0cecc939c5e76e6611fa50bf2dfc77b33dad1339d60fd564950","height":640789,"targetDeadline":31536000}
	// 22:58:55 found deadline=14337735 nonce=16737781616095649804 for account: 7955621360090688183 file: 7955621360090688183_16737781616095649280_3808
	// 22:59:24 found deadline=8684993 nonce=16737781616037910778 for account: 7955621360090688183 file: 7955621360090688183_16737781616037855232_114440
	// 22:59:49 found deadline=5048801 nonce=13119679609447394366 for account: 7955621360090688183 file: 7955621360090688183_13119679609447389920_30512

	auto const height = 640789;
	auto currentSignature = HexString::from("cfff797ad97df0cecc939c5e76e6611fa50bf2dfc77b33dad1339d60fd564950");
	uint64_t currentBaseTarget = 63956;
	uint64_t account_nr = 7955621360090688183;

	{
		uint64_t nonce_nr = 16737781616095649804;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1389
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/14337735); // burst:279646746383234
	}
	{
		uint64_t nonce_nr = 16737781616037910778;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1389
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/8684993); // burst:219207207532813
	}
	{
		uint64_t nonce_nr = 13119679609447394366;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1389
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/5048801); // burst:268406527820901
	}
}

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst640790differsFromClassicBurst) {
	// 640790
	// 23:00:08 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Tue, 09 Jul 2019 21:00:08 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":63182,"generationSignature":"376198795f6c0bf351f803af4f5827fc6bdef58f68730f55179a46c46ffe3b45","height":640790,"targetDeadline":31536000}
	// 23:00:20 found deadline=28535232 nonce=13119679609472713094 for account: 7955621360090688183 file: 7955621360090688183_13119679609472684368_30512
	// 23:00:21 found deadline=9694344 nonce=16737781616095177518 for account: 7955621360090688183 file: 7955621360090688183_16737781616095077880_114440
	// 23:00:40 found deadline=1642333 nonce=16737781616071735303 for account: 7955621360090688183 file: 7955621360090688183_16737781616071731968_114440

	auto const height = 640790;
	auto currentSignature = HexString::from("376198795f6c0bf351f803af4f5827fc6bdef58f68730f55179a46c46ffe3b45");
	uint64_t currentBaseTarget = 63182;
	uint64_t account_nr = 7955621360090688183;

	{
		uint64_t nonce_nr = 13119679609472713094;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1186
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/28535232); // burst:60710794689875
	}
	{
		uint64_t nonce_nr = 16737781616095177518;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1186
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/9694344); // burst:249647812299895
	}
	{
		uint64_t nonce_nr = 16737781616071735303;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1186
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_NE(deadline, /*dcminer:*/1642333); // burst:198609049111880
	}
}

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst640788withGenSigFixup) {
	// 640788
	// 22:57:36 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Tue, 09 Jul 2019 20:57:37 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":62772,"generationSignature":"26e31a80a2302e5bf8ecc132d8c6af761341e3f332afd886022cec162047b356","height":640788,"targetDeadline":31536000}
	// 22:57:40 found deadline=5816864 nonce=13692243649592120299 for account: 7955621360090688183 file: 7955621360090688183_13692243649592081192_114440

	auto const height = 640788;
	auto serverSignature = HexString::from("26e31a80a2302e5bf8ecc132d8c6af761341e3f332afd886022cec162047b356");
	uint64_t currentBaseTarget = 62772;
	uint64_t account_nr = 7955621360090688183;

	auto currentSignature = diskcoin_generate_gensig_aes128(height, serverSignature);

	{
		uint64_t nonce_nr = 13692243649592120299;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 24
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 5816864);
	}
}

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst640789withGenSigFixup) {
	// 640789
	// 22:58:46 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Tue, 09 Jul 2019 20:58:46 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":63956,"generationSignature":"cfff797ad97df0cecc939c5e76e6611fa50bf2dfc77b33dad1339d60fd564950","height":640789,"targetDeadline":31536000}
	// 22:58:55 found deadline=14337735 nonce=16737781616095649804 for account: 7955621360090688183 file: 7955621360090688183_16737781616095649280_3808
	// 22:59:24 found deadline=8684993 nonce=16737781616037910778 for account: 7955621360090688183 file: 7955621360090688183_16737781616037855232_114440
	// 22:59:49 found deadline=5048801 nonce=13119679609447394366 for account: 7955621360090688183 file: 7955621360090688183_13119679609447389920_30512

	auto const height = 640789;
	auto serverSignature = HexString::from("cfff797ad97df0cecc939c5e76e6611fa50bf2dfc77b33dad1339d60fd564950");
	uint64_t currentBaseTarget = 63956;
	uint64_t account_nr = 7955621360090688183;

	auto currentSignature = diskcoin_generate_gensig_aes128(height, serverSignature);

	{
		uint64_t nonce_nr = 16737781616095649804;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1389
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 14337735);
	}
	{
		uint64_t nonce_nr = 16737781616037910778;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1389
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 8684993);
	}
	{
		uint64_t nonce_nr = 13119679609447394366;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1389
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 5048801);
	}
}

TEST(RnD_BurstMath_CalcDeadline_QueLap, dccONburst640790withGenSigFixup) {
	// 640790
	// 23:00:08 * GMI: Received: HTTP/1.0 200 OK\r\nX-Ratelimit-Limit: 3\r\nX-Ratelimit-Remaining: 2\r\nX-Ratelimit-Reset: 1\r\nDate: Tue, 09 Jul 2019 21:00:08 GMT\r\nContent-Length: 151\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n{"baseTarget":63182,"generationSignature":"376198795f6c0bf351f803af4f5827fc6bdef58f68730f55179a46c46ffe3b45","height":640790,"targetDeadline":31536000}
	// 23:00:20 found deadline=28535232 nonce=13119679609472713094 for account: 7955621360090688183 file: 7955621360090688183_13119679609472684368_30512
	// 23:00:21 found deadline=9694344 nonce=16737781616095177518 for account: 7955621360090688183 file: 7955621360090688183_16737781616095077880_114440
	// 23:00:40 found deadline=1642333 nonce=16737781616071735303 for account: 7955621360090688183 file: 7955621360090688183_16737781616071731968_114440

	auto const height = 640790;
	auto serverSignature = HexString::from("376198795f6c0bf351f803af4f5827fc6bdef58f68730f55179a46c46ffe3b45");
	uint64_t currentBaseTarget = 63182;
	uint64_t account_nr = 7955621360090688183;

	auto currentSignature = diskcoin_generate_gensig_aes128(height, serverSignature);

	{
		uint64_t nonce_nr = 13119679609472713094;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1186
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 28535232);
	}
	{
		uint64_t nonce_nr = 16737781616095177518;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1186
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 9694344);
	}
	{
		uint64_t nonce_nr = 16737781616071735303;
		uint32_t scoop_nr = BurstMath::calculate_scoop(height, currentSignature.data()); // 1186
		unsigned long long deadline = BurstMath::calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature.data());
		EXPECT_EQ(deadline, 1642333);
	}
}

//- sample data obtainer from running dcminer on burst pool: voiplanparty.com
