#pragma once
#include "common-pragmas.h"

#include "common.h"

class Output_PlainText : public IUserInterface
{
public:
	~Output_PlainText();
	Output_PlainText();

	// TODO: add v-overload taking va_list
	void printToConsole(
		int colorPair, bool printTimestamp, bool leadingNewLine,
		bool trailingNewLine, bool fillLine, const wchar_t * format,
		...
	) override;
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
	void debugRoundTime(
		double theads_time
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

	void printConnQuality(int ncoins, std::wstring const& connQualInfo) override;
	void printScanProgress(int ncoins, std::wstring const& connQualInfo,
		unsigned long long bytesRead, unsigned long long round_size,
		double thread_time, double threads_speed,
		unsigned long long deadline) override;

	void printFileStats(std::map<std::string, t_file_stats> const & fileStats);

	bool currentlyDisplayingCorruptedPlotFiles();
	void showNewVersion(std::string version);

	int bm_wgetchMain(); //get input vom main window

private:

	struct ConsoleOutput {
		bool leadingNewLine, trailingNewLine;
		bool fillLine;
		std::wstring message;
	};

	std::mutex mConsoleQueue;
	std::list<ConsoleOutput> consoleQueue;

	int prevNCoins21 = 0;
	std::wstring leadingSpace21 = IUserInterface::make_leftpad_for_networkstats(21, 0);

	int prevNCoins94 = 0;
	std::wstring leadingSpace94 = IUserInterface::make_leftpad_for_networkstats(94, 0);

	std::mutex mLog;
	std::list<std::wstring> loggingQueue;

	bool interruptConsoleWriter = false;
	std::thread consoleWriter;

	void _progressWriter();
	void _consoleWriter();
};
