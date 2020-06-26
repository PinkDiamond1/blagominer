#pragma once
#include "common-pragmas.h"

#include "common.h"

void Csv_Init();
void Csv_Fail(std::shared_ptr<t_coin_info> coin, const unsigned long long height, const std::wstring& file, const unsigned long long baseTarget,
	const unsigned long long netDiff, const unsigned long long nonce, const unsigned long long deadlineSent,
	const unsigned long long deadlineConfirmed, const std::string& response);
void Csv_Submitted(std::shared_ptr<t_coin_info> coin, const unsigned long long height, const unsigned long long baseTarget,
	const unsigned long long netDiff, const double roundTime, const bool completedRound, const unsigned long long deadline);
