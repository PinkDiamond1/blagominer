#include "loggerCsv.h"

bool existsFile(const std::wstring& name) {
	struct _stat64i32 buffer;
	return (_wstat(name.c_str(), &buffer) == 0);
}

static const char* headersFail = "Timestamp epoch;Timestamp local;Height;File;baseTarget;Network difficulty;Nonce;Deadline sent;Deadline confirmed;Response\n";
static const char* headersSubmitted = "Timestamp epoch;Timestamp local;Height;baseTarget;Network difficulty;Round time;Completed round; Deadline\n";

void Csv_initFilenames(
	LogFileInfo& coinlogfileinfo,
	const wchar_t* prefix, const wchar_t* coinName)
{
	std::wstring nameW(coinName);
	coinlogfileinfo.filename = prefix + (L"-" + nameW) + L".csv";
}

void Csv_logFileInit(
	std::shared_ptr<t_coin_info> coin, LogFileInfo& coinlogfileinfo,
	const char* headers)
{
	if ((coin->mining->enable || coin->network->enable_proxy) && !existsFile(coinlogfileinfo.filename))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogfileinfo.mutex);
		Log(L"Writing headers to %S", coinlogfileinfo.filename.c_str());
		FILE * pFile;
		_wfopen_s(&pFile, coinlogfileinfo.filename.c_str(), L"a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, headers);
			fclose(pFile);
		}
		else
		{
			Log(L"Failed to open %S", coinlogfileinfo.filename.c_str());
		}
	}
}

void Csv_Init()
{
	if (!loggingConfig.enableCsv)
		return;

	Log(L"Initializing csv logging.");

	for (auto& coin : allcoins)
	{
		Csv_initFilenames(coin->logging.failed, L"fail", coin->coinname.c_str());
		Csv_initFilenames(coin->logging.submitted, L"stat", coin->coinname.c_str());

		Csv_logFileInit(coin, coin->logging.failed, headersFail);
		Csv_logFileInit(coin, coin->logging.submitted, headersSubmitted);
	}
}

void Csv_logFailed(
	std::shared_ptr<t_coin_info> coin, CoinLogFiles& coinlog,
	std::time_t rawtime, char* timeDate,
	const unsigned long long height, const std::string& file, const unsigned long long baseTarget,
	const unsigned long long netDiff, const unsigned long long nonce, const unsigned long long deadlineSent,
	const unsigned long long deadlineConfirmed, const std::string& response)
{
	if (coin->mining->enable || coin->network->enable_proxy)
	{
		std::lock_guard<std::mutex> lockGuard(coinlog.failed.mutex);
		FILE* pFile;
		_wfopen_s(&pFile, coinlog.failed.filename.c_str(), L"a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, "%llu;%s;%llu;%s;%llu;%llu;%llu;%llu;%llu;%s\n", (unsigned long long)rawtime, timeDate, height, file.c_str(), baseTarget, netDiff,
				nonce, deadlineSent, deadlineConfirmed, response.c_str());
			fclose(pFile);
			return;
		}
		else
		{
			Log(L"Failed to open %S", coinlog.failed.filename.c_str());
			return;
		}
	}
}

void Csv_Fail(
	std::shared_ptr<t_coin_info> coin,
	const unsigned long long height, const std::string& file, const unsigned long long baseTarget,
	const unsigned long long netDiff, const unsigned long long nonce, const unsigned long long deadlineSent,
	const unsigned long long deadlineConfirmed, const std::string& response)
{
	if (!loggingConfig.enableCsv)
		return;

	std::time_t rawtime = std::time(nullptr);
	char timeDate[20];
	getLocalDateTime(rawtime, timeDate);

	Csv_logFailed(
		coin, coin->logging,
		rawtime, timeDate,
		height, file.c_str(), baseTarget, netDiff,
		nonce, deadlineSent, deadlineConfirmed, response.c_str());
}

void Csv_logSubmitted(
	std::shared_ptr<t_coin_info> coin, CoinLogFiles& coinlog,
	std::time_t rawtime, char* timeDate,
	const unsigned long long height, const unsigned long long baseTarget, const unsigned long long netDiff,
	const double roundTime, const bool completedRound, const unsigned long long deadline)
{
	if (coin->mining->enable || coin->network->enable_proxy)
	{
		std::lock_guard<std::mutex> lockGuard(coinlog.submitted.mutex);
		FILE* pFile;
		_wfopen_s(&pFile, coinlog.submitted.filename.c_str(), L"a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, "%llu;%s;%llu;%llu;%llu;%.1f;%s;%llu\n", (unsigned long long)rawtime, timeDate, height, baseTarget,
				netDiff, roundTime, completedRound ? "true" : "false", deadline);
			fclose(pFile);
			return;
		}
		else
		{
			Log(L"Failed to open %S", coinlog.submitted.filename.c_str());
			return;
		}
	}
}

void Csv_Submitted(
	std::shared_ptr<t_coin_info> coin,
	const unsigned long long height, const unsigned long long baseTarget, const unsigned long long netDiff,
	const double roundTime, const bool completedRound, const unsigned long long deadline)
{
	if (!loggingConfig.enableCsv)
		return;

	std::time_t rawtime = std::time(nullptr);
	char timeDate[20];
	getLocalDateTime(rawtime, timeDate);

	Csv_logSubmitted(
		coin, coin->logging,
		rawtime, timeDate,
		height, baseTarget,
		netDiff, roundTime, completedRound ? "true" : "false", deadline);
}
