#pragma once
#include "common-pragmas.h"
#include "stdafx.h" // for SYSTEMTIME

#include <mutex>
#include <list>
#include <ctime>
#include <time.h>


//logger variables
extern std::mutex mLog;
extern std::list<std::wstring> loggingQueue;

extern bool loggingInitialized;

//logger functions
void Log_init(void);
void Log_end(void);
std::string Log_server(char const *const strLog);

template<typename ... Args>
void Log(const wchar_t * format, Args ... args)
{
	if (!loggingInitialized) {
		return;
	}

	// TODO: extract, deduplicate
	SYSTEMTIME cur_time;
	GetLocalTime(&cur_time);

	std::wstring timeMil = (
		std::wostringstream()
		<< std::setw(2) << std::setfill(L'0') << cur_time.wHour
		<< ':'
		<< std::setw(2) << std::setfill(L'0') << cur_time.wMinute
		<< ':'
		<< std::setw(2) << std::setfill(L'0') << cur_time.wSecond
		<< '.'
		<< std::setw(3) << std::setfill(L'0') << cur_time.wMilliseconds
		).str();
	//-TODO: extract, deduplicate

	int size = swprintf(nullptr, 0, format, args ...) + 1;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, format, args ...);
	{
		std::lock_guard<std::mutex> lockGuard(mLog);
		loggingQueue.push_back(timeMil + L" " + std::wstring(buf.get(), buf.get() + size - 1));
	}
};
