#pragma once

#include "../../common.h"

namespace BurstMath
{
	uint32_t calculate_scoop(uint64_t height, uint8_t *gensig);

	uint64_t calcdeadline(uint64_t account_nr, uint64_t nonce_nr, uint32_t scoop_nr, uint64_t currentBaseTarget, uint8_t currentSignature[32]);
}
