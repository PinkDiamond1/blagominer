#include "BurstMath.h"

#include "../../shabal.h"

// I think I took this code from https://github.com/PoC-Consortium/Nogrod/blob/master/pkg/burstmath/libs/burstmath.c
// but really, over last few days I saw so many copies of it that I'm not sure right now
// one thing sure, I had to adapt it a little since some macros and constants were not present in this project
// and I also added comments separating generating-the-plot from the actual deadline-calculation
namespace BurstMath
{
	uint32_t calculate_scoop(
		uint64_t height
		, std::array<uint8_t, 32> const & gensig
	) {
		sph_shabal_context sc;
		uint8_t new_gensig[32];

		sph_shabal256_init(&sc);
		sph_shabal256(&sc, gensig.data(), 32);

		uint8_t height_swapped[8];
		//* SET_NONCE made it in one go, I didn't want to copy a macro, so for now: copy&swap
		memcpy_s(height_swapped, sizeof(height_swapped), &height, 8);
		//*byte swap time!
		std::swap(*(height_swapped + 0), *(height_swapped + 7));
		std::swap(*(height_swapped + 1), *(height_swapped + 6));
		std::swap(*(height_swapped + 2), *(height_swapped + 5));
		std::swap(*(height_swapped + 3), *(height_swapped + 4));
		//*!emit paws etyb

		sph_shabal256(&sc, (unsigned char *)&height_swapped, sizeof(height_swapped));
		sph_shabal256_close(&sc, new_gensig);

		return ((new_gensig[30] & 0x0F) << 8) | new_gensig[31];
	}

	uint64_t calcdeadline(
		uint64_t account_nr
		, uint64_t nonce_nr
		, uint32_t scoop_nr
		, uint64_t currentBaseTarget
		, std::array<uint8_t, 32> const & currentSignature
		, bool simulate_messedup_POC1_POC2
	) {
		std::unique_ptr<std::array<uint8_t, 32>> ptr1;
		std::unique_ptr<std::array<uint8_t, 32>> ptr2;
		return calcdeadline(account_nr, nonce_nr, scoop_nr, currentBaseTarget, currentSignature, ptr1, ptr2, simulate_messedup_POC1_POC2);
	}

	uint64_t calcdeadline(
		uint64_t account_nr
		, uint64_t nonce_nr
		, uint32_t scoop_nr
		, uint64_t currentBaseTarget
		, std::array<uint8_t, 32> const & currentSignature
		, std::unique_ptr<std::array<uint8_t, 32>>& scoopLow
		, std::unique_ptr<std::array<uint8_t, 32>>& scoopHigh
		, bool simulate_messedup_POC1_POC2
	) {
		///* simulate reading ACCOUNT:NONCENR:SCOOPNR from files == generate the part of the plot..
		const auto NONCE_SIZE = 4096 * 64;
		const auto HASH_SIZE = 32;
		const auto HASH_CAP = 4096;
		const auto SCOOP_SIZE = 64;

		uint8_t scoop[SCOOP_SIZE];

		if (scoopLow == nullptr || scoopHigh == nullptr)
		{
			char final[32];
			char gendata[16 + NONCE_SIZE];

			//* SET_NONCE made it in one go, I didn't want to copy a macro, so for now: copy&swap
			memcpy_s(gendata + NONCE_SIZE + 0, sizeof(gendata), &account_nr, 8);
			//*byte swap time!
			std::swap(*(gendata + NONCE_SIZE + 0 + 0), *(gendata + NONCE_SIZE + 0 + 7));
			std::swap(*(gendata + NONCE_SIZE + 0 + 1), *(gendata + NONCE_SIZE + 0 + 6));
			std::swap(*(gendata + NONCE_SIZE + 0 + 2), *(gendata + NONCE_SIZE + 0 + 5));
			std::swap(*(gendata + NONCE_SIZE + 0 + 3), *(gendata + NONCE_SIZE + 0 + 4));
			//*!emit paws etyb

			//* SET_NONCE made it in one go, I didn't want to copy a macro, so for now: copy&swap
			memcpy_s(gendata + NONCE_SIZE + 8, sizeof(gendata) - 8, &nonce_nr, 8);
			//*byte swap time!
			std::swap(*(gendata + NONCE_SIZE + 8 + 0), *(gendata + NONCE_SIZE + 8 + 7));
			std::swap(*(gendata + NONCE_SIZE + 8 + 1), *(gendata + NONCE_SIZE + 8 + 6));
			std::swap(*(gendata + NONCE_SIZE + 8 + 2), *(gendata + NONCE_SIZE + 8 + 5));
			std::swap(*(gendata + NONCE_SIZE + 8 + 3), *(gendata + NONCE_SIZE + 8 + 4));
			//*!emit paws etyb

			sph_shabal_context x;
			size_t len;

			for (int i = NONCE_SIZE; i > 0; i -= HASH_SIZE) {
				sph_shabal256_init(&x);

				len = NONCE_SIZE + 16 - i;
				if (len > HASH_CAP)
					len = HASH_CAP;

				sph_shabal256(&x, (unsigned char *)&gendata[i], len);
				sph_shabal256_close(&x, &gendata[i - HASH_SIZE]);
			}

			sph_shabal256_init(&x);
			sph_shabal256(&x, (unsigned char *)gendata, 16 + NONCE_SIZE);
			sph_shabal256_close(&x, final);

			// XOR with final
			for (int i = 0; i < NONCE_SIZE; i++)
				gendata[i] ^= (final[i % 32]);

			if (!simulate_messedup_POC1_POC2)
			{
				// normal POC2 operation
				memcpy(scoop, gendata + (scoop_nr * SCOOP_SIZE), 32);
				memcpy(scoop + 32, gendata + ((4095 - scoop_nr) * SCOOP_SIZE) + 32, 32);
			}
			else
			{
				// simulate a POC2 data file being read in POC1 mode
				memcpy(scoop, gendata + (scoop_nr * SCOOP_SIZE), 32);
				memcpy(scoop + 32, gendata + (scoop_nr * SCOOP_SIZE) + 32, 32);
			}
			///* now we have the data 'read from the plot files' in SCOOP array

			// sidenote: is it possible to write a `new` expr that returns constant-length array pointer type (`uint8_t(*)[32]`)?
			// RE: as of 'now', apparently not yet (https://stackoverflow.com/questions/19467909/will-the-new-expression-ever-return-a-pointer-to-an-array)
			scoopLow = std::make_unique<std::array<uint8_t, HASH_SIZE>>();
			scoopHigh = std::make_unique<std::array<uint8_t, HASH_SIZE>>();
			memcpy_s(scoopLow.get(), HASH_SIZE, scoop + 0, 32);
			memcpy_s(scoopHigh.get(), HASH_SIZE, scoop + 32, 32);
		}
		else
		{
			if (simulate_messedup_POC1_POC2) throw std::invalid_argument("simulate_messedup_POC1_POC2 option has no sense when providing specific scoop data");
			memcpy_s(scoop + 0, HASH_SIZE, scoopLow.get(), 32);
			memcpy_s(scoop + 32, HASH_SIZE, scoopHigh.get(), 32);
		}

		///* back to orig algo from blago shabal.cpp:procscoop_sph()
		sph_shabal_context xx;
		sph_shabal256_init(&xx); // = init(global_32)
		char sig[32 + 64 + 64];
		char res[32];
		///*
		memcpy_s(sig, sizeof(sig), currentSignature.data(), sizeof(char) * 32);
		memcpy_s(&sig[32], sizeof(sig) - 32, scoop, sizeof(char) * 64);

		sph_shabal256(&xx, (const unsigned char*)sig, 64 + 32);
		sph_shabal256_close(&xx, res);

		unsigned long long *wertung = (unsigned long long*)res;

		unsigned long long deadline = *wertung / currentBaseTarget;

		return deadline;
	}
}
