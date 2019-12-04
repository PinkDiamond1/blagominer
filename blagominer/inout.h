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

struct IOutput_Curses;
class Output_Curses;

extern std::unique_ptr<IOutput_Curses> gui;


struct IOutput_Curses
{
	virtual ~IOutput_Curses() = 0;

	virtual void printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
		bool trailingNewLine, bool fillLine, const wchar_t * format, ...) = 0;

	virtual void printToProgress(const wchar_t * format, ...) = 0;

	virtual bool currentlyDisplayingCorruptedPlotFiles() = 0;

	virtual int bm_wgetchMain() = 0; //get input vom main window

	virtual void showNewVersion(std::string version) = 0;

	virtual void printFileStats(std::map<std::string, t_file_stats>const& fileStats) = 0;
};

class Output_Curses : public IOutput_Curses
{
short win_size_x = 96;
short win_size_y = 60;
bool lockWindowSize = true;

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
	~Output_Curses();
	Output_Curses(short x, short y, bool lock);

// TODO: add v-overload taking va_list
void printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
	bool trailingNewLine, bool fillLine, const wchar_t * format, ...) override;

// TODO: add v-overload taking va_list
void printToProgress(const wchar_t * format, ...) override;

private:
void setupSize(short& x, short& y, bool& lock);
void bm_init();
void bm_end();

public:
bool currentlyDisplayingCorruptedPlotFiles();
private:
bool currentlyDisplayingNewVersion();

public:
int bm_wgetchMain(); //get input vom main window

private:
int bm_wattronC(int color);
int bm_wattroffC(int color);
int bm_wprintwC(const char * output, ...);

void refreshCorrupted();
public:
void showNewVersion(std::string version);

private:
void cropCorruptedIfNeeded(int lineCount);
void resizeCorrupted(int lineCount);
int getRowsCorrupted();

void clearCorrupted();
void clearCorruptedLine();
void clearNewVersion();

void hideCorrupted();

int bm_wmoveC(int line, int column);

void boxCorrupted();

public:
void printFileStats(std::map<std::string, t_file_stats>const& fileStats);

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

	int oldLineCount = -1;
};

std::wstring make_filled_string(int n, wchar_t filler);
std::wstring make_leftpad_for_networkstats(int space, int coins);
