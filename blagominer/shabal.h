#pragma once

#include "blagominer.h"
#include "sph_shabal.h"
#include "mshabal.h"


//gobal shabal contexts, just created once
extern sph_shabal_context global_32;
extern mshabal_context global_128;
extern mshabal256_context global_256;

//optimized contexts
extern mshabal_context_fast global_128_fast;
extern mshabal256_context_fast global_256_fast;

//hashing routines
void procscoop_sph(const unsigned long long nonce, const unsigned long long n, char const *const data, const size_t acc, const std::string &file_name);
void procscoop_sse4(unsigned long long const nonce, unsigned long long const n, char const *const data, size_t const acc, const std::string &file_name);
void procscoop_avx(unsigned long long const nonce, unsigned long long const n, char const *const data, size_t const acc, const std::string &file_name);
void procscoop_avx2(unsigned long long const nonce, unsigned long long const n, char const *const data, size_t const acc, const std::string &file_name);
//fast hashing routines by Johnny
//void procscoop_sse4_fast(unsigned long long const nonce, unsigned long long const n, char const *const data, size_t const acc, const std::string &file_name);
void procscoop_avx_fast(unsigned long long const nonce, unsigned long long const n, char const *const data, size_t const acc, const std::string &file_name);
void procscoop_avx2_fast(unsigned long long const nonce, unsigned long long const n, char const *const data, size_t const acc, const std::string &file_name);

