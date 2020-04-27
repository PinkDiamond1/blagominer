#include "stdafx.h"
#include "worker.h"
#include "reference/burst/BurstMath.h"
#include "hexstring.h"

#include "blagominer.h" // for GetFiles, stopThreads, ..
#include "filemonitor.h" // for increaseReadError, ..
#include "error.h" // for ShowMemErrorExit
#include "accounts.h" // for Get_index_acc
#include "shabal.h" // for procscoop_sse_fast


bool use_boost = false;				// use optimisations if true
size_t cache_size1 = 16384;			// Cache in nonces (1 nonce in scoop = 64 bytes) for native POC
size_t cache_size2 = 262144;		// Cache in nonces (1 nonce in scoop = 64 bytes) for on-the-fly POC conversion
size_t readChunkSize = 16384;		// Size of HDD reads in nonces (1 nonce in scoop = 64 bytes)

void work_i(std::shared_ptr<t_coin_info> coinInfo, std::shared_ptr<t_directory_info> directory, const size_t local_num)
{
	// when the mode is OFFLINE then the miner doesn't scan the files and just generates the plot data on the fly
	// therefore we need a simple way to force the loop to do exactly 1 iteration and disable all normal file-ops
	bool testmodeIgnoresPlotfiles = testmodeConfig.isEnabled && coinInfo->testround2->mode == t_roundreplay_round_test::RoundTestMode::RMT_OFFLINE;

	__int64 start_work_time, end_work_time;
	__int64 start_time_read, end_time_read;

	double sum_time_proc = 0;
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	double const pcFreq = double(li.QuadPart);
	QueryPerformanceCounter((LARGE_INTEGER*)&start_work_time);

	if (use_boost)
	{
		SetThreadIdealProcessor(GetCurrentThread(), (DWORD)(local_num % std::thread::hardware_concurrency()));
	}

	std::string const path_loc_str = directory->dir;

	unsigned long long files_size_per_thread = 0;

	if (!testmodeIgnoresPlotfiles)
	Log(L"Start thread: [%zu] %S", local_num, path_loc_str.c_str());
	else
	Log(L"Start thread: [%zu] testmode:offline", local_num);

	if (directory->files.empty()) {
		bool dummyvar;
		GetFiles(path_loc_str, &directory->files, &dummyvar, true);
	}

	size_t cache_size_local;
	DWORD sectorsPerCluster;
	DWORD bytesPerSector;
	DWORD numberOfFreeClusters;
	DWORD totalNumberOfClusters;
	bool converted = false;
	bool isbfs = false;
	
	//for (auto iter = files.begin(); iter != files.end(); ++iter)
	for (auto iter = testmodeIgnoresPlotfiles ? --directory->files.rend() : directory->files.rbegin(); iter != directory->files.rend(); ++iter)
	{
		// TODO: above: RBEGIN/REND? why? that may totally interfere with @ ordering option..
		// but right now the @ ordering seems to work fine, strance
		// I wonder what would be the effect if it's changed back to normal

		// New block while processing: Stop.
		if (stopThreads)
		{
			worker_progress[local_num].isAlive = false;
			Log(L"[%zu] Reading directory: %S interrupted", local_num, directory->dir.c_str());
			return;
		}

		if (iter->done) {
			Log(L"Skipping file %S, since it has already been processed in the interrupted run.", iter->Name.c_str());
			continue;
		}

		// in online testmode, we don't want to waste time reading all files
		// only nonce/scoop from specific test round does matter, so we can skip all other files
		if (testmodeConfig.isEnabled && !testmodeIgnoresPlotfiles)
		{
			if (iter->Key != coinInfo->testround2->assume_account)
			{
				Log(L"TESTMODE: account mismatch, skipping file: %S (%S)", iter->Name.c_str(), iter->Path.c_str());
				continue;
			}
			if (!(iter->StartNonce <= coinInfo->testround2->assume_nonce && (iter->StartNonce + iter->Nonces) > coinInfo->testround2->assume_nonce))
			{
				Log(L"TESTMODE: nonce range mismatch, skipping file: %S (%S)", iter->Name.c_str(), iter->Path.c_str());
				continue;
			}
		}

		//Log("[%zu] Beginning main loop over files.", local_num);
		std::string filename;
		unsigned long long key, nonce, nonces, stagger, offset, tail;
		bool p2, bfs;
		QueryPerformanceCounter((LARGE_INTEGER*)&start_time_read);
		filename = iter->Name;
		key = iter->Key;
		nonce = iter->StartNonce;
		nonces = iter->Nonces;
		stagger = iter->Stagger;
		offset = iter->Offset;
		// offline testmode assumes POC2 algorithms, so 'assume POC1' option can be used to test POC1/2 mixups
		p2 = iter->P2 && !testmodeIgnoresPlotfiles || testmodeIgnoresPlotfiles && true;
		bfs = iter->BFS;
		tail = 0;

		// TODO: below: now that's a creative way of testing if integer division resulted in nonzero remainder
		// Checking the stagger's nonce count
		if ((double)(nonces % stagger) > DBL_EPSILON && !bfs && !testmodeIgnoresPlotfiles)
		{
			std::thread{ increaseReadError, iter->Name.c_str() }.detach();
			Log(L"File %S (%S) wrong stagger?", iter->Name.c_str(), iter->Path.c_str());
			gui->printToConsole(12, true, false, true, false, L"File %S wrong stagger?", iter->Name.c_str());
		}

		// in testmode, updateCurrentMiningInfo from blagominer.cpp makes sure the `scoop` is properly set up
		const unsigned int scoop = coinInfo->mining->scoop;
		bool POC2 = coinInfo->isPoc2Round();
		if (testmodeConfig.isEnabled && coinInfo->testround1->assume_POC2.has_value())
			POC2 = coinInfo->testround1->assume_POC2.value();
		size_t acc = Get_index_acc(key, coinInfo, getTargetDeadlineInfo(coinInfo));

		if (!testmodeIgnoresPlotfiles)
		{
		// Checking for plot damage
		if (nonces != (iter->Size) / (4096 * 64))
		{
			std::thread{ increaseReadError, iter->Name.c_str() }.detach();
			Log(L"File %S (%S) name/size mismatch.", iter->Name.c_str(), iter->Path.c_str());
			gui->printToConsole(12, true, false, true, false, L"File \"%S\" name/size mismatch", iter->Name.c_str());
			if (nonces != stagger)
				nonces = (((iter->Size) / (4096 * 64)) / stagger) * stagger; //we cut the plot to size and stagger
			else {
				if (scoop > (iter->Size) / (stagger * 64)) //if the number of scoop falls into a damaged, cursed plot, then skip
				{
					gui->printToConsole(12, true, false, true, false, L"Skipped");
					continue;
				}
			}
		}

		//get sector information, set to 4096 for BFS
		if (!bfs) {
			if (!GetDiskFreeSpaceA((iter->Path).c_str(), &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters))
			{
				gui->printToConsole(12, true, false, true, false, L"GetDiskFreeSpace failed"); //BFS
				continue;
			}
		}
		else
		{
			bytesPerSector = 4096;
		}

		// If the stagger in the plot is smaller than the size of the sector - skip
		if ((stagger * 64) < bytesPerSector)
		{
			std::thread{ increaseReadError, iter->Name.c_str() }.detach();
			Log(L"File %S (%S): Stagger (%llu) must be >= %llu", iter->Name.c_str(), iter->Path.c_str(), stagger, bytesPerSector / 64);
			gui->printToConsole(12, true, false, true, false, L"Stagger (%llu) must be >= %llu", stagger, bytesPerSector / 64);
			continue;
		}

		// If the nonce in the plot is smaller than the size of the sector - skip
		if ((nonces * 64) < bytesPerSector)
		{
			std::thread{ increaseReadError, iter->Name.c_str() }.detach();
			Log(L"File %S (%S): Nonces(%llu) must be >= %llu", iter->Name.c_str(), iter->Path.c_str(), nonces, bytesPerSector / 64);
			gui->printToConsole(12, true, false, true, false, L"Nonces (%llu) must be >= %llu", nonces, bytesPerSector / 64);
			continue;
		}

		// If the stagger is not aligned with the sector - we can read by shifting the last stagger backwards (finish)
		if ((stagger % (bytesPerSector / 64)) != 0)
		{
			std::thread{ increaseReadError, iter->Name.c_str() }.detach();
			Log(L"File %S (%S): Stagger (%llu) must be a multiple of %llu", iter->Name.c_str(), iter->Path.c_str(), stagger, bytesPerSector / 64);
			gui->printToConsole(12, true, false, true, false, L"Stagger (%llu) must be a multiple of %llu", stagger, bytesPerSector / 64);
			continue;
		}

		//PoC2 cache size added (shuffling needs more cache)

		// below: this is irrelevant in offline testmode as long as resulting cachesizes are long enough to fit one nonce
		if (p2 != POC2) {
			if ((stagger == nonces) && (cache_size2 < stagger)) cache_size_local = cache_size2;  // optimized plot
			else cache_size_local = stagger; // regular plot
		}
		else {
			if ((stagger == nonces) && (cache_size1 < stagger))
			{
				cache_size_local = cache_size1;  // optimized plot
			}
			else
			{
				if (!bfs) {
					cache_size_local = stagger; // regular plot 
				}
				else
				{
					cache_size_local = cache_size1;  //BFS
				}
			}
		}

		//Align cache_size_local to sector size.
		cache_size_local = (cache_size_local / (size_t)(bytesPerSector / 64)) * (size_t)(bytesPerSector / 64);
		//wprintw(win_main, "round: %llu\n", cache_size_local);

		} //!testmodeIgnoresPlotfiles
		else
		{
			// in offline testmode, only one specific nonce/scoop is generated
			// warning: even though in offline testmode we're interested in only a single nonce data (thus we'd want cache_size_local=1)
			// the CACHE data is passed directly to shabal routines which blindly assume certain data block sizes
			// since HERE we don't want to switch over CPUIDs, we pick a value that covers LONGEST shabal scratchpad memory supported
			cache_size_local = 16; // right now AVX512 has the largest input buffer reqs
			key = coinInfo->testround2->assume_account;
			acc = Get_index_acc(key, coinInfo, getTargetDeadlineInfo(coinInfo));
			nonce = coinInfo->testround2->assume_nonce;
			nonces = 1;
			stagger = 1;
			filename = "offline";
		}

		size_t cache_size_local_backup = cache_size_local;
		char *cache = (char *)VirtualAlloc(nullptr, cache_size_local * 64, MEM_COMMIT, PAGE_READWRITE); //cache thread1
		char *cache2 = (char *)VirtualAlloc(nullptr, cache_size_local * 64, MEM_COMMIT, PAGE_READWRITE); //cache thread2
		char *MirrorCache = nullptr;
		// for offline testmode, data is generated already in desired format, no mirroring needed
		if (p2 != POC2 && !testmodeIgnoresPlotfiles) {
			MirrorCache = (char *)VirtualAlloc(nullptr, cache_size_local * 64, MEM_COMMIT, PAGE_READWRITE); //PoC2 cache
			if (MirrorCache == nullptr) ShowMemErrorExit();
			converted = true;
		}

		if (bfs) isbfs = true;

		if (cache == nullptr) ShowMemErrorExit();
		if (cache2 == nullptr) ShowMemErrorExit();

		if (!testmodeIgnoresPlotfiles)
		Log(L"[%zu] Read file : %S", local_num, iter->Name.c_str());
		else
		Log(L"[%zu] Generating nonce : %llu", local_num, coinInfo->testround2->assume_nonce);

		//wprintw(win_main, "%S \n", str2wstr(iter->Path + iter->Name).c_str());
		HANDLE ifile = INVALID_HANDLE_VALUE;
		if (!testmodeIgnoresPlotfiles)
		{
		if (bfs) {
			ifile = CreateFileA((iter->Path).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, nullptr);
		}
		else {
			ifile = CreateFileA((iter->Path + iter->Name).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, nullptr);
		}
		if (ifile == INVALID_HANDLE_VALUE)
		{
			std::thread{ increaseReadError, iter->Name.c_str() }.detach();
			Log(L"File %S (%S): Error opening. code = %lu", iter->Name.c_str(), iter->Path.c_str(), GetLastError());
			gui->printToConsole(12, true, false, true, false, L"File \"%S\\%S\" error opening. code = %lu", iter->Path.c_str(), iter->Name.c_str(), GetLastError());
			VirtualFree(cache, 0, MEM_RELEASE);
			VirtualFree(cache2, 0, MEM_RELEASE); //Cleanup Thread 2
			if (p2 != POC2) VirtualFree(MirrorCache, 0, MEM_RELEASE); //PoC2 Cleanup
			continue;
		}
		files_size_per_thread += iter->Size;
		}//?testmodeIgnoresPlotfiles

		unsigned long long start, bytes;

		LARGE_INTEGER liDistanceToMove = { 0 };

		//PoC2 Vars
		unsigned long long MirrorStart;

		LARGE_INTEGER MirrorliDistanceToMove = { 0 };
		bool flip = false;

		// in offline testmode, we don't need ANY further reading, one step is enough
		for (unsigned long long n = 0; n < nonces; n += stagger)
		{
			// New block while processing: Stop.
			if (stopThreads)
			{
				worker_progress[local_num].isAlive = false;
				if (!testmodeIgnoresPlotfiles)
				{
				Log(L"[%zu] Reading file: %S interrupted", local_num, iter->Name.c_str());
				CloseHandle(ifile);
				}
				Log(L"[%zu] Freeing caches.", local_num);
				VirtualFree(cache, 0, MEM_RELEASE); //Cleanup Thread 1
				VirtualFree(cache2, 0, MEM_RELEASE); //Cleanup Thread 2
				if (p2 != POC2 && !testmodeIgnoresPlotfiles) VirtualFree(MirrorCache, 0, MEM_RELEASE); //PoC2 Cleanup
				return;
			}

			cache_size_local = cache_size_local_backup;
			//Parallel Processing
			bool err = false;
			bool cont = false;

			start = n * 4096 * 64 + scoop * stagger * 64 + offset * 64 * 64; //BFSstaggerstart + scoopstart + offset
			MirrorStart = n * 4096 * 64 + (4095 - scoop) * stagger * 64 + offset * 64 * 64; //PoC2 Seek possition
			int count = 0;

			//Initial Reading
			if (!testmodeIgnoresPlotfiles)
			{
			// WARNING: th_read may MUTATE cache_size_local when it hits an EOF/etc
			th_read(ifile, start, MirrorStart, &cont, &bytes, &(*iter), &flip, p2, POC2, 0, stagger, &cache_size_local, cache, MirrorCache);
			// WARNING: if this initial read FAILS and sets CONT and trims cache_size_local - nothing handles this condition!
			}
			else
			{
			// generate nonce/scoop data in offline testmode
			// warning: even though in offline testmode we're allocated a larger buffer due to shabal implementation's reqs,
			// the higher value was needed only for VirtualAlloc, and we cannot keep value of `cache_size_local` that high.
			// we DO WANT =1 because the internal loops in shabal implementation actually iterate over this `cache_size_local` value
			// and we DONT WANT to let them run more than a single initial iteration
			cache_size_local_backup = cache_size_local = 1;
			bytes = cache_size_local * 64; // for statistics in th_hash, and for a few error checks below 
			if (!coinInfo->testround2->assume_scoop_low.has_value() || !coinInfo->testround2->assume_scoop_high.has_value()) // scoop data not fully provided?
			{
				// below: right now we don't have any other plotting routines other than those embedded within reference calcdeadline
				// the only result needed are the inspection buffers `scooplow` and `scoophigh`,
				// the whole rest of deadline calculation is irrelevant, so we just pass trash as basetarget and gensig
				auto trash1 = 1llu;
				auto trash2 = std::array<uint8_t, 32>();
				std::unique_ptr<std::array<uint8_t, 32>> scooplow;
				std::unique_ptr<std::array<uint8_t, 32>> scoophigh;
				//offline testmode always generates data in a format correct for shabal implementations
				//if 'assume_POC2' indicates POC1, it means that the configuration asks for POC1/POC2 messup simulation
				//TODO: actually, 'assume_POC2' is such a bad name.. try to invent something better considering its semantical difference in ONLINE/OFFLINE modes
				bool poc1poc2mixup = coinInfo->testround1->assume_POC2.has_value() && coinInfo->testround1->assume_POC2.value() == false;
				auto deadline = BurstMath::calcdeadline(key, nonce, scoop, trash1, trash2, scooplow, scoophigh, poc1poc2mixup);
				memcpy_s(cache + 0, 64 - 0, scooplow->data(), 32);
				memcpy_s(cache + 32, 64 - 32, scoophigh->data(), 32);
			}
			}

			if (testmodeConfig.isEnabled)
				if (coinInfo->testround2->assume_nonce >= (nonce + n + 0) && coinInfo->testround2->assume_nonce < (nonce + n + 0 + cache_size_local))
				{
					size_t nonceIndexInCache = coinInfo->testround2->assume_nonce - (nonce + n + 0);
					size_t nonceOffsetInCache = nonceIndexInCache * 64;
					if (coinInfo->testround2->assume_scoop_low.has_value())
					{
						auto scooplow = HexString::arrayfrom<32>(coinInfo->testround2->assume_scoop_low.value());
						memcpy_s(cache + nonceOffsetInCache + 0, 64, scooplow->data(), 32);
					}
					if (coinInfo->testround2->assume_scoop_high.has_value())
					{
						auto scoophigh = HexString::arrayfrom<32>(coinInfo->testround2->assume_scoop_high.value());
						memcpy_s(cache + nonceOffsetInCache + 32, 64 - 32, scoophigh->data(), 32);
					}
					if (coinInfo->testround2->check_scoop_low.has_value())
					{
						auto expected = coinInfo->testround2->check_scoop_low.value();
						auto scooplow = HexString::string(std::vector<uint8_t>(cache + nonceOffsetInCache + 0, cache + nonceOffsetInCache + 32));
						auto testresult = scooplow == expected;
						coinInfo->testround2->passed_scoop_low = testresult && coinInfo->testround2->passed_scoop_low.value_or(true);

						if (!testresult)
						{
							Log(L"TESTMODE: CHECK ERROR: scoop high chunk differs, nonce: %llu, scoop: %u, file: %S",
								coinInfo->testround2->assume_nonce, scoop, filename.c_str());
							Log(L"SCP: low expected: %S", expected.c_str());
							Log(L"SCP: low actual:   %S", scooplow.c_str());
						}
					}
					if (coinInfo->testround2->check_scoop_high.has_value())
					{
						auto expected = coinInfo->testround2->check_scoop_high.value();
						auto scoophigh = HexString::string(std::vector<uint8_t>(cache + nonceOffsetInCache + 32, cache + nonceOffsetInCache + 64));
						auto testresult = scoophigh == expected;
						coinInfo->testround2->passed_scoop_high = testresult && coinInfo->testround2->passed_scoop_high.value_or(true);

						if (!testresult)
						{
							Log(L"TESTMODE: CHECK ERROR: scoop high chunk differs, nonce: %llu, scoop: %u, file: %S",
								coinInfo->testround2->assume_nonce, scoop, filename.c_str());
							Log(L"SCP: high expected: %S", expected.c_str());
							Log(L"SCP: high actual:   %S", scoophigh.c_str());
						}
					}
				}

			char *cachep;
			unsigned long long i;
			// in offline test mode, this loop will NEVER iterate, because cache_size_local==nonces==stagger
			// WARNING: in += the `cache_size_local_backup` should be used, becase `cache_size_local` could be trimmed by initial th_read
			// RE:WARNING: not! actually, it's stateful - initial `i` is OK as it's the actual number read, then AFTER iteration, `cache_size_local` comes from threaded th_read
			// the only downside (or feature?) of this mechanism is that changes to the `cache_size_local` are monotonic: only downwards, so it might incrementally slowdown
			// the reading and finally could start failing when the buffer is smaller than shabal implementation's reqs
			// TODO: analyze if there's a need to reset `cache_size_local` to `cache_size_local_backup` just before starting threaded th_read
			for (i = cache_size_local; i < min(nonces, stagger); i += cache_size_local)
			{
				// New block while processing: Stop.
				if (stopThreads)
				{
					worker_progress[local_num].isAlive = false;
					Log(L"[%zu] Reading file: %S interrupted", local_num, iter->Name.c_str());
					CloseHandle(ifile);
					Log(L"[%zu] Freeing caches.", local_num);
					VirtualFree(cache, 0, MEM_RELEASE); //Cleanup Thread 1
					VirtualFree(cache2, 0, MEM_RELEASE); //Cleanup Thread 2
					if (p2 != POC2) VirtualFree(MirrorCache, 0, MEM_RELEASE); //PoC2 Cleanup
					return;
				}

				//Threadded Hashing
				err = false;
				if (count % 2 == 0) {
					cachep = cache;
				}
				else {
					cachep = cache2;
				}

				//check if hashing would fail
				if (bytes != cache_size_local * 64)
				{
					std::thread{ increaseReadError, iter->Name.c_str() }.detach();
					Log(L"File %S (%S): Unexpected end of file.", iter->Name.c_str(), iter->Path.c_str());
					gui->printToConsole(12, true, false, true, false, L"Unexpected end of file %S", iter->Name.c_str());
					err = true;
					break;
				}

				std::thread hash(th_hash, coinInfo, filename, &sum_time_proc, local_num, bytes, cache_size_local, i - cache_size_local, nonce, n, cachep, acc);

				cont = false;
				//Threadded Reading
				if (count % 2 == 1) {
					cachep = cache;
				}
				else {
					cachep = cache2;
				}
				// WARNING: th_read may MUTATE cache_size_local when it hits an EOF/etc
				// WARNING: if `cache_size_local` was already broken, should we revert to `cache_size_local_backup` before reading again?
				std::thread read = std::thread(th_read, ifile, start, MirrorStart, &cont, &bytes, &(*iter), &flip, p2, POC2, i, stagger, &cache_size_local, cachep, MirrorCache);

				//Join threads
				hash.join();
				read.join();
				if (testmodeConfig.isEnabled)
					if (coinInfo->testround2->assume_nonce >= (nonce + n + i) && coinInfo->testround2->assume_nonce < (nonce + n + i + cache_size_local))
					{
						size_t nonceIndexInCache = coinInfo->testround2->assume_nonce - (nonce + n + i);
						size_t nonceOffsetInCache = nonceIndexInCache * 64;
						if (coinInfo->testround2->assume_scoop_low.has_value())
						{
							auto scooplow = HexString::arrayfrom<32>(coinInfo->testround2->assume_scoop_low.value());
							memcpy_s(cache + nonceOffsetInCache + 0, 64, scooplow->data(), 32);
						}
						if (coinInfo->testround2->assume_scoop_high.has_value())
						{
							auto scoophigh = HexString::arrayfrom<32>(coinInfo->testround2->assume_scoop_high.value());
							memcpy_s(cache + nonceOffsetInCache + 32, 64 - 32, scoophigh->data(), 32);
						}
						if (coinInfo->testround2->check_scoop_low.has_value())
						{
							auto expected = coinInfo->testround2->check_scoop_low.value();
							auto scooplow = HexString::string(std::vector<uint8_t>(cache + nonceOffsetInCache + 0, cache + nonceOffsetInCache + 32));
							auto testresult = scooplow == expected;
							coinInfo->testround2->passed_scoop_low = testresult && coinInfo->testround2->passed_scoop_low.value_or(true);

							if (!testresult)
							{
								Log(L"TESTMODE: CHECK ERROR: scoop high chunk differs, nonce: %llu, scoop: %u, file: %S",
									coinInfo->testround2->assume_nonce, scoop, filename.c_str());
								Log(L"SCP: low expected: %S", expected.c_str());
								Log(L"SCP: low actual:   %S", scooplow.c_str());
							}
						}
						if (coinInfo->testround2->check_scoop_high.has_value())
						{
							auto expected = coinInfo->testround2->check_scoop_high.value();
							auto scoophigh = HexString::string(std::vector<uint8_t>(cache + nonceOffsetInCache + 32, cache + nonceOffsetInCache + 64));
							auto testresult = scoophigh == expected;
							coinInfo->testround2->passed_scoop_high = testresult && coinInfo->testround2->passed_scoop_high.value_or(true);

							if (!testresult)
							{
								Log(L"TESTMODE: CHECK ERROR: scoop high chunk differs, nonce: %llu, scoop: %u, file: %S",
									coinInfo->testround2->assume_nonce, scoop, filename.c_str());
								Log(L"SCP: high expected: %S", expected.c_str());
								Log(L"SCP: high actual:   %S", scoophigh.c_str());
							}
						}
					}
				count += 1;
				if (cont) continue;
				// TODO: above: `cont` is set when error occurs, but the loop ends here anyways, what's the point in that conditional continue here?!
				// I can only guess that it was meant not for this (scoop reading), but for one of the outer loops, but the, it could not be meant
				// for the 'directory/file' loop, as it SKIPS the cleanups.. so, only 'nonce' loop is left as an option.. time for some archeology?
			}

			//Final Hashing
			//check if hashing would fail
			if (!err) {
				if (bytes != cache_size_local * 64 && !testmodeIgnoresPlotfiles)
				{
					std::thread{ increaseReadError, iter->Name.c_str() }.detach();
					Log(L"Unexpected end of file %S (%S)", iter->Name.c_str(), path_loc_str.c_str());
					gui->printToConsole(12, true, false, true, false, L"Unexpected end of file %S", iter->Name.c_str());
					err = true;
				}
			}
			if (!err)
			{
				if (count % 2 == 0) {
					th_hash(coinInfo, filename, &sum_time_proc, local_num, bytes, cache_size_local, i - cache_size_local, nonce, n, cache, acc);
				}
				else {
					th_hash(coinInfo, filename, &sum_time_proc, local_num, bytes, cache_size_local, i - cache_size_local, nonce, n, cache2, acc);
				}
			}

		}
		QueryPerformanceCounter((LARGE_INTEGER*)&end_time_read);
		//bfs seek optimisation
		if (bfs && iter == directory->files.rend() && !testmodeIgnoresPlotfiles) {
			//assuming physical hard disk mid at scoop 1587
			start = 0 * 4096 * 64 + 1587 * stagger * 64 + 6 * 64 * 64;
			if (!SetFilePointerEx(ifile, liDistanceToMove, nullptr, FILE_BEGIN))
			{
				std::thread{ increaseReadError, iter->Name.c_str() }.detach();
				Log(L"BFS seek optimisation: error SetFilePointerEx. code = %lu. File: %S (%S)", GetLastError(), iter->Name.c_str(), path_loc_str.c_str());
				gui->printToConsole(12, true, false, true, false, L"BFS seek optimisation: error SetFilePointerEx. code = %lu", GetLastError());
			}
		}
		iter->done = true;
		if (!testmodeIgnoresPlotfiles)
		{
		if (pcFreq != 0) {
			Log(L"[%zu] Close file: %S (%S) [@ %llu ms]", local_num, iter->Name.c_str(), path_loc_str.c_str(), (long long unsigned)((double)(end_time_read - start_time_read) * 1000 / pcFreq));
		}
		else {
			Log(L"[%zu] Close file: %S (%S)", local_num, iter->Name.c_str(), path_loc_str.c_str());
		}
		CloseHandle(ifile);
		}
		Log(L"[%zu] Freeing caches.", local_num);
		VirtualFree(cache, 0, MEM_RELEASE);
		VirtualFree(cache2, 0, MEM_RELEASE); //Cleanup Thread 2
		if (p2 != POC2 && !testmodeIgnoresPlotfiles) VirtualFree(MirrorCache, 0, MEM_RELEASE); //PoC2 Cleanup
	}
	//Log("[%zu] All files processed.", local_num);
	worker_progress[local_num].isAlive = false;
	QueryPerformanceCounter((LARGE_INTEGER*)&end_work_time);

	//if (use_boost) SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	// in offline testmode I turn it off just to not have to analyze&patch it
	if (use_debug && pcFreq != 0 && !testmodeIgnoresPlotfiles)
	{
		double cpuTime = sum_time_proc / pcFreq;
		double workTime = (double)(end_work_time - start_work_time) / pcFreq;
		if (workTime > 0) // hide stats for idle workers
		{
			auto tag = isbfs ? L"BFS" :
				converted ? L"POC1<>POC2" :
				L"";

			gui->debugWorkerStats(
				tag,
				path_loc_str,
				cpuTime, workTime,
				files_size_per_thread
			);
		}
	}
	directory->done = true;
	if (!testmodeIgnoresPlotfiles)
	Log(L"[%zu] Finished directory %S.", local_num, path_loc_str.c_str());
	else
	Log(L"[%zu] Finished testmode:offline.", local_num);
	return;
}

void th_read(HANDLE ifile, unsigned long long const start, unsigned long long const MirrorStart, bool * const cont, unsigned long long * const bytes, t_files const * const iter, bool * const flip, bool p2, bool POC2, unsigned long long const i, unsigned long long const stagger, size_t * const cache_size_local, char * const cache, char * const MirrorCache) {
	if (i + *cache_size_local > stagger)
	{
		*cache_size_local = stagger - i;  // the remainder

#ifdef __AVX512F__
		if (*cache_size_local < 16) // these checks are related to shabal-impl's scratchpad memory size = number of 'vectorized' nonces/scoops
		{
			gui->printToConsole(12, false, false, true, false, L"WARNING: %llu", *cache_size_local);
		}
#else
#ifdef __AVX2__
		if (*cache_size_local < 8) // these checks are related to shabal-impl's scratchpad memory size = number of 'vectorized' nonces/scoops
		{
			gui->printToConsole(12, false, false, true, false, L"WARNING: %llu", *cache_size_local);
		}
#else
#ifdef __AVX__
		if (*cache_size_local < 4) // these checks are related to shabal-impl's scratchpad memory size = number of 'vectorized' nonces/scoops
		{
			gui->printToConsole(12, false, false, true, false, L"WARNING: %llu", *cache_size_local);
		}
#endif
#endif
#endif
	}

	DWORD b = 0;
	DWORD Mirrorb = 0;
	LARGE_INTEGER liDistanceToMove;
	LARGE_INTEGER MirrorliDistanceToMove;

	//alternating front scoop back scoop
	if (*flip) goto poc2read;


	//POC1 scoop read
poc1read:
	*bytes = 0;
	b = 0;
	liDistanceToMove.QuadPart = start + i * 64;
	if (!SetFilePointerEx(ifile, liDistanceToMove, nullptr, FILE_BEGIN))
	{
		gui->printToConsole(12, false, false, true, false, L"Error SetFilePointerEx. code = %lu", GetLastError());
		*cont = true;
		return;
	}
	do {
		if (!ReadFile(ifile, &cache[*bytes], (DWORD)(min(readChunkSize, *cache_size_local - *bytes / 64) * 64), &b, NULL))
		{
			gui->printToConsole(12, false, false, true, false, L"Error P1 ReadFile. code = %lu", GetLastError());
			break;
		}
		*bytes += b;

	} while (*bytes < *cache_size_local * 64);
	if (*flip) goto readend;

poc2read:
	//PoC2 mirror scoop read
	if (p2 != POC2) {
		*bytes = 0;
		Mirrorb = 0;
		MirrorliDistanceToMove.QuadPart = MirrorStart + i * 64;
		if (!SetFilePointerEx(ifile, MirrorliDistanceToMove, nullptr, FILE_BEGIN))
		{
			gui->printToConsole(12, false, false, true, false, L"Error SetFilePointerEx. code = %lu", GetLastError());
			*cont = true;
			return;
		}
		do {
			if (!ReadFile(ifile, &MirrorCache[*bytes], (DWORD)(min(readChunkSize, *cache_size_local - *bytes / 64) * 64), &Mirrorb, NULL))
			{
				gui->printToConsole(12, false, false, true, false, L"Error P2 ReadFile. code = %lu", GetLastError());
				break;
			}
			*bytes += Mirrorb;
		} while (*bytes < *cache_size_local * 64);
	}
	if (*flip) goto poc1read;
readend:
	*flip = !*flip;

	//PoC2 Merge data to Cache
	if (p2 != POC2) {
		for (unsigned long t = 0; t < *bytes; t += 64) {
			memcpy(&cache[t + 32], &MirrorCache[t + 32], 32); //copy second hash to correct place.
		}
	}
}

void th_hash(std::shared_ptr<t_coin_info> coin, std::string const& filename, double * const sum_time_proc, const size_t &local_num, unsigned long long const bytes, size_t const cache_size_local, unsigned long long const i, unsigned long long const nonce, unsigned long long const n, char const * const cache, size_t const acc) {
	LARGE_INTEGER li;
	LARGE_INTEGER start_time_proc;
	QueryPerformanceCounter(&start_time_proc);

#ifdef __AVX512F__
	procscoop_avx512_fast(coin, n + nonce + i, cache_size_local, cache, acc, filename);// Process block AVX2
#else
#ifdef __AVX2__
	procscoop_avx2_fast(coin, n + nonce + i, cache_size_local, cache, acc, filename);// Process block AVX2
#else
	#ifdef __AVX__
		procscoop_avx_fast(coin, n + nonce + i, cache_size_local, cache, acc, filename);// Process block AVX
	#else
			procscoop_sse_fast(coin, n + nonce + i, cache_size_local, cache, acc, filename);// Process block SSE
		//	procscoop_sph(coin, n + nonce + i, cache_size_local, cache, acc, filename);// Process block SPH, please uncomment one of the two when compiling    
	#endif
#endif
#endif
	QueryPerformanceCounter(&li);
	*sum_time_proc += (double)(li.QuadPart - start_time_proc.QuadPart);
	worker_progress[local_num].Reads_bytes += bytes;
}
