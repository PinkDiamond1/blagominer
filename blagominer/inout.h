#pragma once
#include "common-pragmas.h"

#include "common.h"

#undef  MOUSE_MOVED
#ifndef PDC_DLL_BUILD
# define PDC_DLL_BUILD
#endif
#ifndef PDC_WIDE
# define PDC_WIDE
#endif
#include <curses.h>

class Output_Curses : public IUserInterface
{
public:
	~Output_Curses();
	Output_Curses(short x, short y, bool lock);

	// TODO: add v-overload taking va_list
	void printToConsole(
		int colorPair, bool printTimestamp, bool leadingNewLine,
		bool trailingNewLine, bool fillLine, const wchar_t * format,
		...
	) override;

	void printHeadlineTitle(
		std::wstring const& appname,
		std::wstring const& version,
		bool elevated
	) override;
	void printWallOfCredits(
		std::vector<std::wstring> const& history
	) override;
	void printHWInfo(
		hwinfo const& info
	) override;

	void printPlotsStart() override;
	void printPlotsInfo(char const* const directory, unsigned nfiles, unsigned long long size) override;
	void printPlotsEnd(unsigned long long total_size) override;

	void printThreadActivity(
		std::wstring const& coinName,
		std::wstring const& threadKind,
		std::wstring const& threadAction
	) override;

	void debugWorkerStats(
		std::wstring const& specialReadMode,
		std::string const& directory,
		double proc_time, double work_time,
		unsigned long long files_size_per_thread
	) override;
	void printWorkerDeadlineFound(
		unsigned long long account_id,
		std::wstring const& coinname,
		unsigned long long deadline
	) override;
	void printScanProgress(
		int ncoins, std::wstring const& connQualInfo,
		unsigned long long bytesRead, unsigned long long round_size,
		double thread_time, double threads_speed,
		unsigned long long deadline
	) override;

	void printNetworkHostResolution(
		std::wstring const& lookupWhat,
		std::wstring const& coinName,
		std::string const& remoteName,
		std::vector<char> const& resolvedIP,
		std::string const& remotePost,
		std::string const& remotePath
	) override;
	void printNetworkProxyDeadlineReceived(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		char const (&clientAddr)[22]
	) override;
	void debugNetworkProxyDeadlineAcked(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		char const (&clientAddr)[22]
	) override;
	void debugNetworkDeadlineDiscarded(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		unsigned long long targetDeadline
	) override;
	void printNetworkDeadlineSent(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline
	) override;
	void printNetworkDeadlineConfirmed(
		bool with_timespan,
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline
	) override;
	void debugNetworkTargetDeadlineUpdated(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long targetDeadline
	) override;
	void printConnQuality(
		int ncoins, std::wstring const& connQualInfo
	) override;

	void debugRoundTime(
		double theads_time
	) override;
	void printBlockEnqueued(
		unsigned long long currentHeight,
		std::wstring const& coinname,
		bool atEnd, bool noQueue
	) override;
	void printRoundInterrupt(
		unsigned long long currentHeight,
		std::wstring const& coinname
	) override;
	void printRoundChangeInfo(bool isResumingInterrupted,
		unsigned long long currentHeight,
		std::wstring const& coinname,
		unsigned long long currentBaseTarget,
		unsigned long long currentNetDiff,
		bool isPoc2Round
	) override;

	void printFileStats(std::map<std::string, t_file_stats> const & fileStats);

	bool currentlyDisplayingCorruptedPlotFiles();
	void showNewVersion(std::string version);

	int bm_wgetchMain(); //get input vom main window

private:

	struct ConsoleOutput {
		int colorPair = 0;
		bool leadingNewLine = false, trailingNewLine = false;
		bool fillLine = false;
		std::wstring message;
	};

	short win_size_x = 96;
	short win_size_y = 60;
	bool lockWindowSize = true;

	std::mutex mConsoleQueue;
	std::mutex mProgressQueue;
	std::mutex mConsoleWindow;
	std::list<ConsoleOutput> consoleQueue;
	std::list<std::wstring> progressQueue;

	int prevNCoins21 = 0;
	std::wstring leadingSpace21 = IUserInterface::make_leftpad_for_networkstats(21, 0);

	int prevNCoins94 = 0;
	std::wstring leadingSpace94 = IUserInterface::make_leftpad_for_networkstats(94, 0);

	std::mutex mLog;
	std::list<std::wstring> loggingQueue;

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

	int oldLineCount = -1;

	void bm_init();
	void bm_end();
	int bm_wattronC(int color);
	int bm_wattroffC(int color);
	int bm_wprintwC(const char * output, ...);
	int bm_wmoveC(int line, int column);

	void setupSize(short& x, short& y, bool& lock);
	bool currentlyDisplayingNewVersion();
	void clearNewVersion();

	void refreshCorrupted();
	void cropCorruptedIfNeeded(int lineCount);
	void resizeCorrupted(int lineCount);
	int getRowsCorrupted();
	void clearCorrupted();
	void clearCorruptedLine();
	void hideCorrupted();
	void boxCorrupted();

	void _progressWriter();
	void _consoleWriter();
};
