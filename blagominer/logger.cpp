#include "logger.h"

#include "common.h"

FILE * fp_Log = nullptr;

std::mutex mLog;
std::list<std::wstring> loggingQueue;
std::thread writer;
bool interruptWriter = false;
bool loggingInitialized = false;

void _writer()
{
	while (!interruptWriter || !loggingQueue.empty()) {
		if (!loggingQueue.empty()) {
			std::wstring str;
			{
				std::lock_guard<std::mutex> lockGuard(mLog);
				str = loggingQueue.front();
				loggingQueue.pop_front();
			}
			fwprintf_s(fp_Log, L"%s\n", str.c_str());
			fflush(fp_Log);
		}
		else {
			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

void Log_init(void)
{
	if (loggingConfig.enableLogging)
	{
		loggingInitialized = true;
		std::stringstream ss;
		if (CreateDirectory(L"Logs", nullptr) == ERROR_PATH_NOT_FOUND)
		{
			gui->printToConsole(12, false, false, true, false, L"CreateDirectory failed (%lu)", GetLastError());
			loggingInitialized = false;
			loggingConfig.enableLogging = false;
			return;
		}
		std::time_t rawtime = std::time(nullptr);
		char timeDate[20];
		getLocalDateTime(rawtime, timeDate, "-");

		ss << "Logs\\" << timeDate << ".log";
		std::string filename = ss.str();
		if ((fp_Log = _fsopen(filename.c_str(), "wt", _SH_DENYNO)) == NULL)
		{
			gui->printToConsole(12, false, false, true, false, L"LOG: file openinig error");
			loggingInitialized = false;
			loggingConfig.enableLogging = false;
		}
		else {
			writer = std::thread(_writer);
			Log(version.c_str());
		}
	}
}

void Log_end(void)
{
	interruptWriter = true;
	if (writer.joinable())
	{
		writer.join();
	}
	if (fp_Log != nullptr)
	{
		fflush(fp_Log);
		fclose(fp_Log);
		fp_Log = nullptr;
	}
}

// TODO: impl seems to be nonsensically optimized - measure and cleanup
// 1) HA/HF/memcpy to prevent overallocation in std::string? it's a TEMPORARY string for wtf's sake
// 2) ZEROMEMORY the whole disposable buffer, while simple one '\0' terminator is enough,
// and it exactly knows where to put it as it initially strlen()s the input data
// 3) it initially strlen()s the input data, while in 99% of cases the caller already knows it
// 4) allocating disposable buffer of 2*LEN size just to ensure enough spaces for escaped characters,
// while it strlen()s the input already so 1 full scan of the data is already performed,
// so an EXACT COUNT of escapes can be easily gathered at almost no cost
std::string Log_server(char const *const strLog)
{
	size_t len_str = strlen(strLog);
	if ((len_str> 0) && loggingConfig.enableLogging)
	{
		std::vector<char, heap_allocator<char>> Msg_log(len_str * 2 + 1, theHeap);

		for (size_t i = 0, j = 0; i<len_str; i++, j++)
		{
			if (strLog[i] == '\r')
			{
				Msg_log[j] = '\\';
				j++;
				Msg_log[j] = 'r';
			}
			else
				if (strLog[i] == '\n')
				{
					Msg_log[j] = '\\';
					j++;
					Msg_log[j] = 'n';
				}
				else
					if (strLog[i] == '%')
					{
						Msg_log[j] = '%';
						j++;
						Msg_log[j] = '%';
					}
					else Msg_log[j] = strLog[i];
		}

		std::string ret(Msg_log.data());

		return ret;
	}
	return "";
}
