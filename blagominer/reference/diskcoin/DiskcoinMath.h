#pragma once

#include <array>
#include <memory>
#include <vector>

std::unique_ptr<std::array<uint8_t, 32>> diskcoin_generate_gensig_aes128(size_t serverHeight, std::array<uint8_t, 32> const& serverGenSig);
