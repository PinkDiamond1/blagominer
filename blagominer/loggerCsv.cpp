#include "loggerCsv.h"

struct LogFileInfo {
	std::string filename;
	std::mutex mutex;
};

struct CoinLogFiles {
	LogFileInfo failed;
	LogFileInfo submitted;
};

CoinLogFiles coinlogs[2];

bool existsFile(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

void Csv_Init()
{
	if (!loggingConfig.enableCsv) {
		return;
	}

	std::wstring burstNameW(coinNames[BURST]);
	std::wstring bhdNameW(coinNames[BHD]);

	std::string burstName(burstNameW.begin(), burstNameW.end());
	std::string bhdName(bhdNameW.begin(), bhdNameW.end());

	coinlogs[BURST].failed.filename = "fail-" + burstName + ".csv";
	coinlogs[BHD].failed.filename = "fail-" + bhdName + ".csv";
	coinlogs[BURST].submitted.filename = "stat-" + burstName + ".csv";
	coinlogs[BHD].submitted.filename = "stat-" + bhdName + ".csv";

	Log(L"Initializing csv logging.");
	const char* headersFail = "Timestamp epoch;Timestamp local;Height;File;baseTarget;Network difficulty;Nonce;Deadline sent;Deadline confirmed;Response\n";
	const char* headersSubmitted = "Timestamp epoch;Timestamp local;Height;baseTarget;Network difficulty;Round time;Completed round; Deadline\n";
	if ((burst->mining->enable || burst->network->enable_proxy) && !existsFile(coinlogs[BURST].failed.filename))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BURST].failed.mutex);
		Log(L"Writing headers to %S", coinlogs[BURST].failed.filename.c_str());
		FILE * pFile;
		fopen_s(&pFile, coinlogs[BURST].failed.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, headersFail);
			fclose(pFile);
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BURST].failed.filename.c_str());
		}
	}
	if ((burst->mining->enable || burst->network->enable_proxy) && !existsFile(coinlogs[BURST].submitted.filename))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BURST].submitted.mutex);
		Log(L"Writing headers to %S", coinlogs[BURST].submitted.filename.c_str());
		FILE * pFile;
		fopen_s(&pFile, coinlogs[BURST].submitted.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, headersSubmitted);
			fclose(pFile);
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BURST].submitted.filename.c_str());
		}
	}

	if ((bhd->mining->enable || burst->network->enable_proxy) && !existsFile(coinlogs[BHD].failed.filename))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BHD].failed.mutex);
		Log(L"Writing headers to %S", coinlogs[BHD].failed.filename.c_str());
		FILE * pFile;
		fopen_s(&pFile, coinlogs[BHD].failed.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, headersFail);
			fclose(pFile);
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BHD].failed.filename.c_str());
		}
	}
	if ((bhd->mining->enable || bhd->network->enable_proxy) && !existsFile(coinlogs[BHD].submitted.filename))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BHD].submitted.mutex);
		Log(L"Writing headers to %S", coinlogs[BHD].submitted.filename.c_str());
		FILE * pFile;
		fopen_s(&pFile, coinlogs[BHD].submitted.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, headersSubmitted);
			fclose(pFile);
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BHD].submitted.filename.c_str());
		}
	}
}

void Csv_Fail(Coins coin, const unsigned long long height, const std::string& file, const unsigned long long baseTarget,
	const unsigned long long netDiff, const unsigned long long nonce, const unsigned long long deadlineSent,
	const unsigned long long deadlineConfirmed, const std::string& response)
{
	if (!loggingConfig.enableCsv) {
		return;
	}
	std::time_t rawtime = std::time(nullptr);
	char timeDate[20];
	getLocalDateTime(rawtime, timeDate);

	FILE * pFile;
	if (coin == BURST && (burst->mining->enable || burst->network->enable_proxy))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BURST].failed.mutex);
		fopen_s(&pFile, coinlogs[BURST].failed.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, "%llu;%s;%llu;%s;%llu;%llu;%llu;%llu;%llu;%s\n", (unsigned long long)rawtime, timeDate, height, file.c_str(), baseTarget, netDiff,
				nonce, deadlineSent, deadlineConfirmed, response.c_str());
			fclose(pFile);
			return;
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BURST].failed.filename.c_str());
			return;
		}
	}
	else if (coin == BHD && (bhd->mining->enable || bhd->network->enable_proxy))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BHD].failed.mutex);
		fopen_s(&pFile, coinlogs[BHD].failed.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, "%llu;%s;%llu;%s;%llu;%llu;%llu;%llu;%llu;%s\n", (unsigned long long)rawtime, timeDate, height, file.c_str(), baseTarget, netDiff,
				nonce, deadlineSent, deadlineConfirmed, response.c_str());
			fclose(pFile);
			return;
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BHD].failed.filename.c_str());
			return;
		}
	}

}

void Csv_Submitted(Coins coin, const unsigned long long height, const unsigned long long baseTarget, const unsigned long long netDiff,
	const double roundTime, const bool completedRound, const unsigned long long deadline)
{
	if (!loggingConfig.enableCsv) {
		return;
	}
	std::time_t rawtime = std::time(nullptr);
	char timeDate[20];
	getLocalDateTime(rawtime, timeDate);

	FILE * pFile;
	if (coin == BURST && (burst->mining->enable || burst->network->enable_proxy))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BURST].submitted.mutex);
		fopen_s(&pFile, coinlogs[BURST].submitted.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, "%llu;%s;%llu;%llu;%llu;%.1f;%s;%llu\n", (unsigned long long)rawtime, timeDate, height, baseTarget,
				netDiff, roundTime, completedRound ? "true" : "false", deadline);
			fclose(pFile);
			return;
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BURST].submitted.filename.c_str());
			return;
		}
	}
	else if (coin == BHD && (bhd->mining->enable || bhd->network->enable_proxy))
	{
		std::lock_guard<std::mutex> lockGuard(coinlogs[BHD].submitted.mutex);
		fopen_s(&pFile, coinlogs[BHD].submitted.filename.c_str(), "a+t");
		if (pFile != nullptr)
		{
			fprintf(pFile, "%llu;%s;%llu;%llu;%llu;%.1f;%s;%llu\n", (unsigned long long)rawtime, timeDate, height, baseTarget,
				netDiff, roundTime, completedRound ? "true" : "false", deadline);
			fclose(pFile);
			return;
		}
		else
		{
			Log(L"Failed to open %S", coinlogs[BHD].submitted.filename.c_str());
			return;
		}
	}

}