#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "blagominer.h"
#include "network.h"

TEST(LoadConfig, Basic_POC2StartBlock) {
	Document document;
	{
		rapidjson::Document::AllocatorType& docAllocator = document.GetAllocator();
		document.SetObject();

		rapidjson::Value n_coin(rapidjson::kObjectType);
		document.AddMember("coin:ABC", n_coin, docAllocator);

		rapidjson::Value n_coin2(rapidjson::kObjectType);
		document.AddMember("coin:BurstLikeCoin", n_coin2, docAllocator);

		rapidjson::Value n_coin3(rapidjson::kObjectType);
		n_coin3.AddMember("POC2StartBlock", 789, docAllocator);
		document.AddMember("coin:DEF", n_coin3, docAllocator);
	}

	// use load_config so it invokes init_mining_info+init_coinNetwork for each coin
	// we need to use load_config, because it calculates default values for PRIORITY
	load_config(document);

	// however, 'load_config' puts the resuls in global static structures
	auto& firstCoin = allcoins[0];
	EXPECT_EQ(firstCoin->coinname, L"ABC");
	EXPECT_EQ(firstCoin->mining->POC2StartBlock, 0);

	auto& secondCoin = allcoins[1];
	EXPECT_EQ(secondCoin->coinname, L"BurstLikeCoin");
	EXPECT_EQ(secondCoin->mining->POC2StartBlock, 502000);

	auto& thirdCoin = allcoins[2];
	EXPECT_EQ(thirdCoin->coinname, L"DEF");
	EXPECT_EQ(thirdCoin->mining->POC2StartBlock, 789);
}

TEST(LoadConfig, Basic_Priority) {
	Document document;
	{
		rapidjson::Document::AllocatorType& docAllocator = document.GetAllocator();
		document.SetObject();

		rapidjson::Value n_coin(rapidjson::kObjectType);
		document.AddMember("coin:ABC", n_coin, docAllocator);

		rapidjson::Value n_coin2(rapidjson::kObjectType);
		document.AddMember("coin:BurstLikeCoin", n_coin2, docAllocator);

		rapidjson::Value n_coin3(rapidjson::kObjectType);
		n_coin3.AddMember("Priority", 123, docAllocator);
		document.AddMember("coin:DEF", n_coin3, docAllocator);
	}

	// use load_config so it invokes init_mining_info+init_coinNetwork for each coin
	// we need to use load_config, because it calculates default values for PRIORITY
	load_config(document);

	// however, 'load_config' puts the resuls in global static structures
	auto& firstCoin = allcoins[0];
	EXPECT_EQ(firstCoin->coinname, L"ABC");
	EXPECT_EQ(firstCoin->mining->priority, 1);

	auto& secondCoin = allcoins[1];
	EXPECT_EQ(secondCoin->coinname, L"BurstLikeCoin");
	EXPECT_EQ(secondCoin->mining->priority, 0);

	auto& thirdCoin = allcoins[2];
	EXPECT_EQ(thirdCoin->coinname, L"DEF");
	EXPECT_EQ(thirdCoin->mining->priority, 123);
}

TEST(LoadConfig, Basic_Mode) {
	Document document;
	{
		rapidjson::Document::AllocatorType& docAllocator = document.GetAllocator();
		document.SetObject();

		rapidjson::Value n_coin(rapidjson::kObjectType);
		document.AddMember("coin:ABC", n_coin, docAllocator);

		rapidjson::Value n_coin2(rapidjson::kObjectType);
		n_coin2.AddMember("Mode", "solo", docAllocator);
		document.AddMember("coin:DEF", n_coin2, docAllocator);
	}

	// since now we call loadCoinConfig instead of load_config, results are in local structures
	// but we need to ensure these are properly initialized first
	auto coin1 = std::make_shared<t_coin_info>();
	{
		init_mining_info(coin1, L"ABC", -1, -1);
		init_coinNetwork(coin1);
	}
	auto coin2 = std::make_shared<t_coin_info>();
	{
		init_mining_info(coin2, L"DEF", -1, -1);
		init_coinNetwork(coin2);
	}

	loadCoinConfig(document, "coin:ABC", coin1);
	loadCoinConfig(document, "coin:DEF", coin2);

	EXPECT_EQ(coin1->coinname, L"ABC");
	EXPECT_EQ(coin1->mining->miner_mode, 1);

	EXPECT_EQ(coin2->coinname, L"DEF");
	EXPECT_EQ(coin2->mining->miner_mode, 0);
}

// init_logging_config()
// document -> blagominer:loggingConfig
// Log_init();
// document -> blagominer:allcoins |
//		init_mining_info(c)
//		init_coinNetwork(c)
//		loadCoinConfig(document, coinNodeName, coin) -> blagominer:coins
//			EnableMining => coin->mining->enable
//			Priority => coin->mining->priority
// std::sort(coins.begin(), coins.end(), OrderingByMiningPriority());
// document -> worker:cache_size1
// document -> worker:cache_size2
// document -> worker:readChunkSize
// document -> blagominer:use_wakeup
// document -> filemonitor:showCorruptedPlotFiles
// document -> common:ignoreSuspectedFastBlocks
// document -> blagominer:hddWakeUpTimer
// document -> bfs:bfsTOCOffset
// document -> blagominer:use_debug
// document -> common:checkForUpdateInterval
// document -> worker:use_boost
// document -> inout:win_size_x
// document -> inout:win_size_y
// document -> inout:lockWindowSize
//-document -> gpu_devices:use_gpu_platform
//-document -> gpu_devices:use_gpu_device
