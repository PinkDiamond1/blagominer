#pragma once
#include "common-pragmas.h"

#include <map>
#include <string>
#include <mutex>
#include "inout.h"

extern bool showCorruptedPlotFiles;

void increaseMatchingDeadline(std::string file);
void increaseConflictingDeadline(std::shared_ptr<t_coin_info> coin, unsigned long long height, std::string file);
void increaseReadError(std::string file);
void resetFileStats();
void printFileStats();