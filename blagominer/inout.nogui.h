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

	// TODO: add v-overload taking va_list
	void printToProgress(const wchar_t * format, ...) override;

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

	std::mutex mLog;
	std::list<std::wstring> loggingQueue;

	bool interruptConsoleWriter = false;
	std::thread consoleWriter;

	void _progressWriter();
	void _consoleWriter();
};
