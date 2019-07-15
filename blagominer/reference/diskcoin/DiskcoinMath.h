#pragma once

#include <vector>

std::vector<uint8_t> diskcoin_generate_gensig_aes128(size_t serverHeight, std::vector<uint8_t> const& serverGenSig);
