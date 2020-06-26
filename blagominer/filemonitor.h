#pragma once
#include "common-pragmas.h"

#include "common.h"

extern bool showCorruptedPlotFiles;

void increaseMatchingDeadline(std::wstring file);
void increaseConflictingDeadline(std::shared_ptr<t_coin_info> coin, unsigned long long height, std::wstring file);
void increaseReadError(std::wstring file);
void resetFileStats();
void printFileStats();
