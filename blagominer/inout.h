#pragma once
#include "common.h"

#include <mutex>
#include <list>
#include <ctime>
#include <time.h>
#include "logger.h"

#undef  MOUSE_MOVED
#ifndef PDC_DLL_BUILD
# define PDC_DLL_BUILD
#endif
#ifndef PDC_WIDE
# define PDC_WIDE
#endif
#include <curses.h>

class Output_Curses;

extern std::unique_ptr<Output_Curses> gui;


class Output_Curses
{
short win_size_x = 96;
short win_size_y = 60;

struct ConsoleOutput {
	int colorPair;
	bool leadingNewLine;
	bool fillLine;
	std::wstring message;
};


std::mutex mConsoleQueue;
std::mutex mProgressQueue;
public:std::mutex mConsoleWindow;	/*mConsoleWindow made public for filemonitor.cpp*/private:
std::list<ConsoleOutput> consoleQueue;
std::list<std::wstring> progressQueue;


std::mutex mLog;
std::list<std::wstring> loggingQueue;


public:

template<typename ... Args>
void printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
	bool trailingNewLine, bool fillLine, const wchar_t * format, Args ... args)
{
	std::wstring message;
	SYSTEMTIME cur_time;
	GetLocalTime(&cur_time);
	wchar_t timeBuff[9];
	wchar_t timeBuffMil[13];
	swprintf(timeBuff, sizeof(timeBuff), L"%02d:%02d:%02d", cur_time.wHour, cur_time.wMinute, cur_time.wSecond);
	swprintf(timeBuffMil, sizeof(timeBuff), L"%02d:%02d:%02d.%03d", cur_time.wHour, cur_time.wMinute, cur_time.wSecond, cur_time.wMilliseconds);
	std::wstring time = timeBuff;
	std::wstring timeMil = timeBuffMil;

	if (printTimestamp) {
		
		message = timeBuff;
		message += L" ";
	}

	int size = swprintf(nullptr, 0, format, args ...) + 1;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, format, args ...);
	message += std::wstring(buf.get(), buf.get() + size - 1);
	{
		std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
		consoleQueue.push_back({
			colorPair,
			leadingNewLine,
			fillLine,
			message });
		if (trailingNewLine) {
			consoleQueue.push_back({
			colorPair,
			false,
			false,
			L"\n" });
		}
	}
	{
		std::lock_guard<std::mutex> lockGuard(mLog);
		loggingQueue.push_back(timeMil + L" " + std::wstring(buf.get(), buf.get() + size - 1));
	}
};

template<typename ... Args>
void printToProgress(const wchar_t * format, Args ... args)
{
	std::wstring message;
	
	size_t size = swprintf(nullptr, 0, format, args ...) + 1;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, format, args ...);
	message += std::wstring(buf.get(), buf.get() + size - 1);
	{
		std::lock_guard<std::mutex> lockGuard(mProgressQueue);
		progressQueue.push_back(message);
	}
};


void setupSize(short& x, short& y);
void bm_init();
void bm_end();

bool currentlyDisplayingCorruptedPlotFiles();
bool currentlyDisplayingNewVersion();

int bm_wgetchMain(); //get input vom main window

int bm_wattronC(int color);
int bm_wattroffC(int color);
int bm_wprintwC(const char * output, ...);

void refreshCorrupted();
void showNewVersion(std::string version);

void cropCorruptedIfNeeded(int lineCount);
void resizeCorrupted(int lineCount);
int getRowsCorrupted();

void clearCorrupted();
void clearCorruptedLine();
void clearNewVersion();

void hideCorrupted();

int bm_wmoveC(int line, int column);

void boxCorrupted();


private:

	int minimumWinMainHeight = 5;
	const short progress_lines = 3;
	const short new_version_lines = 3;
	WINDOW * win_main;
	WINDOW * win_progress;
	WINDOW * win_corrupted;
	WINDOW * win_new_version;

	std::thread consoleWriter;
	std::thread progressWriter;
	bool interruptConsoleWriter = false;

	void _progressWriter();
	void _consoleWriter();
};

std::wstring make_filled_string(int n, wchar_t filler);
std::wstring make_leftpad_for_networkstats(int space, int coins);
