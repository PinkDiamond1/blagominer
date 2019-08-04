#pragma once
#include "InstructionSet.h"
#include "bfs.h"
#include "network.h"
#include "shabal.h"
#include "common.h"
#include "filemonitor.h"
#include "updateChecker.h"
#include "elevate.h"
#include "volume_ntfs.h"

// miner
extern volatile bool stopThreads;
extern unsigned long long total_size;			// sum of all local plot file sizes
extern std::vector<std::string> paths_dir;      // plot paths

//miner config items
extern bool use_debug;							// output debug information if true

#include "worker.h"

//headers
size_t GetFiles(std::string str, std::vector <t_files> *p_files, bool* bfsDetected, bool forActualFileReading);
