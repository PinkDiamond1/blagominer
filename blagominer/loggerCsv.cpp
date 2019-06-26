#include "loggerCsv.h"

struct LogFileInfo {
	std::string filename;
	std::mutex mutex;
};

struct CoinLogFiles {
	LogFileInfo failed;
	LogFileInfo submitted;
};

static CoinLogFiles coinlogs[2];

bool existsFile(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

static const char* headersFail = "Timestamp epoch;Timestamp local;Height;File;baseTarget;Network difficulty;Nonce;Deadline sent;Deadline confirmed;Response\n";
static const char* headersSubmitted = "Timestamp epoch;Timestamp local;Height;baseTarget;Network difficulty;Round time;Completed round; Deadline\n";

void Csv_initFilenames(
	LogFileInfo& coinlogfileinfo,
	const char* prefix, const wchar_t* coinName)
{
	std::wstring nameW(coinName);
	std::string name(nameW.begin(), nameW.end());
	coinlogfileinfo.filename = prefix + ("-" + name) + ".csv";
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
		fopen_s(&pFile, coinlogfileinfo.filename.c_str(), "a+t");
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
		Csv_initFilenames(coinlogs[coin->coin].failed, "fail", coin->coinname.c_str());
		Csv_initFilenames(coinlogs[coin->coin].submitted, "stat", coin->coinname.c_str());

		Csv_logFileInit(coin, coinlogs[coin->coin].failed, headersFail);
		Csv_logFileInit(coin, coinlogs[coin->coin].submitted, headersSubmitted);
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
		fopen_s(&pFile, coinlog.failed.filename.c_str(), "a+t");
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
	Coins coin,
	const unsigned long long height, const std::string& file, const unsigned long long baseTarget,
	const unsigned long long netDiff, const unsigned long long nonce, const unsigned long long deadlineSent,
	const unsigned long long deadlineConfirmed, const std::string& response)
{
	if (!loggingConfig.enableCsv)
		return;

	std::time_t rawtime = std::time(nullptr);
	char timeDate[20];
	getLocalDateTime(rawtime, timeDate);

	if (coin == BURST)
		Csv_logFailed(
			burst, coinlogs[BURST],
			rawtime, timeDate,
			height, file.c_str(), baseTarget, netDiff,
			nonce, deadlineSent, deadlineConfirmed, response.c_str());

	if (coin == BHD)
		Csv_logFailed(
			bhd, coinlogs[BHD],
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
		fopen_s(&pFile, coinlog.submitted.filename.c_str(), "a+t");
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
	Coins coin,
	const unsigned long long height, const unsigned long long baseTarget, const unsigned long long netDiff,
	const double roundTime, const bool completedRound, const unsigned long long deadline)
{
	if (!loggingConfig.enableCsv)
		return;

	std::time_t rawtime = std::time(nullptr);
	char timeDate[20];
	getLocalDateTime(rawtime, timeDate);

	if (coin == BURST)
		Csv_logSubmitted(
			burst, coinlogs[BURST],
			rawtime, timeDate,
			height, baseTarget,
			netDiff, roundTime, completedRound ? "true" : "false", deadline);

	if (coin == BHD)
		Csv_logSubmitted(
			bhd, coinlogs[BHD],
			rawtime, timeDate,
			height, baseTarget,
			netDiff, roundTime, completedRound ? "true" : "false", deadline);
}