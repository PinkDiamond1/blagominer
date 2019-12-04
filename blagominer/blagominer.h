#pragma once
#include "common-pragmas.h"

#include "common.h"

// miner
extern volatile bool stopThreads;
extern unsigned long long total_size;			// sum of all local plot file sizes
extern std::vector<std::string> paths_dir;      // plot paths

//miner config items
extern bool use_debug;							// output debug information if true

#include "worker.h"

//headers
size_t GetFiles(std::string str, std::vector <t_files> *p_files, bool* bfsDetected, bool forActualFileReading);

void init_mining_info(std::shared_ptr<t_coin_info> coin, std::wstring name, size_t priority, unsigned long long poc2start);
void init_logging_config();
void init_gui_config();

void loadCoinConfig(Document const & document, std::string section, std::shared_ptr<t_coin_info> coin);

std::vector<char, heap_allocator<char>> load_config_file(wchar_t const *const filename);
Document load_config_json(std::vector<char, heap_allocator<char>> const& json_);
int load_config(Document const& document);

// load_testmode_config
