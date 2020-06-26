#include "blagominer.h"

// TODO: diagnose why can't I make them all CONST here
// why it worked perfectly fine when versionMajor/etc were in COMMON.H/CPP

// app identity
std::wstring appname = L"PoC multi miner";

// blago version
unsigned int versionMajor = 2;
unsigned int versionMinor = 300004;
unsigned int versionRevision = 2;

#ifdef __AVX512F__
std::wstring versionSuffix = L"_AVX512";
#else
#ifdef __AVX2__
std::wstring versionSuffix = L"_AVX2";
#else
#ifdef __AVX__
std::wstring versionSuffix = L"_AVX";
#else
std::wstring versionSuffix = L"_SSE";
//	std::string versionSuffix = "";
#endif
#endif 
#endif 
std::wstring version = std::to_wstring(versionMajor) + L"." + std::to_wstring(versionMinor) + L"." + std::to_wstring(versionRevision) + versionSuffix;
