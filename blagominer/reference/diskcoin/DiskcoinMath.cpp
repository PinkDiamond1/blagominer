#include "DiskcoinMath.h"

#include <sstream>

#include "../../hexstring.h"

#include "ctaes.h"

/*
std::vector<uint8_t> diskcoin_generate_gensig_binload(size_t serverHeight, std::vector<uint8_t>& serverGenSig)
{
	std::string modpath = "D:\\w\\q\\dcminer\\dcminer\\dcminer-unpacked.exe";
	HMODULE module = LoadLibraryA(modpath.c_str());

	size_t offs_code_hasher1 = 0x000000000000C910;
	size_t offs_code_hasher2 = 0x000000000000CC70;

	// warning: that's `hasher1` and `hasher2`, I just came up with a bit better names
	auto initContext = reinterpret_cast<void(*)(uint8_t* output, char const* input)>((uint8_t*)module + offs_code_hasher1);
	auto hash128bits = reinterpret_cast<void(*)(uint8_t const* key, void* unknown, uint8_t const* input, uint8_t* output)>((uint8_t*)module + offs_code_hasher2);

	std::ostringstream cvt("                ");
	cvt << "DISKCOIN";
	cvt << serverHeight;
	std::string seed = cvt.str();

	std::vector<uint8_t> context(176, 'X');
	initContext(context.data(), seed.c_str());

	std::vector<uint8_t> newGenSig(0x20, 'X');
	hash128bits(context.data(), 0, newGenSig.data() + 0x00, serverGenSig.data() + 0x00);
	hash128bits(context.data(), 0, newGenSig.data() + 0x10, serverGenSig.data() + 0x10);

	FreeLibrary(module);

	return newGenSig;
}
*/

std::unique_ptr<std::array<uint8_t, 32>> diskcoin_generate_gensig_aes128(size_t serverHeight, std::array<uint8_t, 32> const & serverGenSig)
{
	auto cappedHeight = serverHeight <= 99999999 ? serverHeight : 99999999; // no space for more than 8 digits!

	std::ostringstream cvt("                ");
	cvt << "DISKCOIN";
	cvt << cappedHeight;
	std::string seed = cvt.str();

	AES128_ctx ctx;
	AES128_init(&ctx, (unsigned char*)seed.c_str());

	auto newGenSig = std::make_unique<std::array<uint8_t, 0x20>>();

	AES128_decrypt(&ctx, 1, newGenSig->data() + 0x00, serverGenSig.data() + 0x00);
	AES128_decrypt(&ctx, 1, newGenSig->data() + 0x10, serverGenSig.data() + 0x10);

	return newGenSig;
}
