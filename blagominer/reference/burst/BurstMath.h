#pragma once
#include "../../common.h"

namespace BurstMath
{
	uint32_t calculate_scoop(
		uint64_t height
		, std::array<uint8_t, 32> const & gensig
	);

	uint64_t calcdeadline(
		uint64_t account_nr
		, uint64_t nonce_nr
		, uint32_t scoop_nr
		, uint64_t currentBaseTarget
		, std::array<uint8_t, 32> const & currentSignature
		, bool simulate_messedup_POC1_POC2 = false
	);

	uint64_t calcdeadline(
		uint64_t account_nr
		, uint64_t nonce_nr
		, uint32_t scoop_nr
		, uint64_t currentBaseTarget
		, std::array<uint8_t, 32> const & currentSignature
		, std::unique_ptr<std::array<uint8_t, 32>>& scoopLow
		, std::unique_ptr<std::array<uint8_t, 32>>& scoopHigh
		, bool simulate_messedup_POC1_POC2 = false
	);
}
