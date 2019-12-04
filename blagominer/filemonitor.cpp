#include "filemonitor.h"

std::mutex mFileStats;

std::map<std::string, t_file_stats> fileStats = std::map<std::string, t_file_stats>();
bool showCorruptedPlotFiles = true;

bool statsChanged = false;
int oldLineCount = -1;
const std::string header = "File name                                             +DLs      -DLs       I/O";

void increaseMatchingDeadline(std::string file) {
	if (!showCorruptedPlotFiles) {
		return;
	}
	std::lock_guard<std::mutex> lockGuard(mFileStats);
	statsChanged = true;
	++fileStats[file].matchingDeadlines;
}

void increaseConflictingDeadline(std::shared_ptr<t_coin_info> coin, unsigned long long height, std::string file) {
	if (!showCorruptedPlotFiles) {
		return;
	}

	Log(L"increaseConflictingDeadline %s", coin->coinname.c_str());
	bool log = true;
	if (ignoreSuspectedFastBlocks) {
		// Wait to see if there is a new block incoming.
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::seconds(5));

		if (height != coin->mining->currentHeight) {
			Log(L"increaseConflictingDeadline %s: Not counting this conflicting deadline, as the cause is most probably a fast block. ",
				coin->coinname.c_str());
			log = false;
		}

	}
	if (log) {
		std::lock_guard<std::mutex> lockGuard(mFileStats);
		statsChanged = true;
		++fileStats[file].conflictingDeadlines;
	}
}

void increaseReadError(std::string file) {
	if (!showCorruptedPlotFiles) {
		return;
	}
	std::lock_guard<std::mutex> lockGuard(mFileStats);
	statsChanged = true;
	++fileStats[file].readErrors;
}

void resetFileStats() {
	if (!showCorruptedPlotFiles || !gui->currentlyDisplayingCorruptedPlotFiles()) {
		return;
	}
	std::lock_guard<std::mutex> lockGuard(mFileStats);
	statsChanged = true;
	fileStats.clear();
}

void printFileStats() {
	if (!showCorruptedPlotFiles || !statsChanged) {
		return;
	}
	std::lock_guard<std::mutex> lockGuardFileStats(mFileStats);
	auto lockGuardConsoleWindow(gui->lock_outputDevice());
	statsChanged = false;
	int lineCount = 0;
	for (auto& element : fileStats) {
		if (element.second.conflictingDeadlines > 0 || element.second.readErrors > 0) {
			++lineCount;
		}
	}
	
	if (lineCount == 0 && gui->currentlyDisplayingCorruptedPlotFiles()) {
		gui->hideCorrupted();
		gui->resizeCorrupted(0);
		gui->refreshCorrupted();
		return;
	}
	else if (lineCount == 0) {
		return;
	}

	// Increase for header, border and for clear message.
	lineCount += 4;

	if (lineCount != oldLineCount) {
		gui->clearCorrupted();
		gui->resizeCorrupted(lineCount);
		oldLineCount = lineCount;
	}
	gui->refreshCorrupted();

	lineCount = 1;
	gui->bm_wmoveC(lineCount++, 1);
	gui->bm_wprintwC("%s", header.c_str(), 0);
	
	for (auto& element : fileStats) {
		if (element.second.conflictingDeadlines > 0 || element.second.readErrors > 0) {
			gui->bm_wattronC(14);
			gui->bm_wmoveC(lineCount, 1);
			gui->bm_wprintwC("%s %s", toStr(element.first, 46).c_str(), toStr(element.second.matchingDeadlines, 11).c_str(), 0);
			if (element.second.conflictingDeadlines > 0) {
				gui->bm_wattronC(4);
			}
			gui->bm_wprintwC(" %s", toStr(element.second.conflictingDeadlines, 9).c_str(), 0);
			gui->bm_wattroffC(4);
			gui->bm_wattronC(14);
			if (element.second.readErrors > 0) {
				gui->bm_wattronC(4);
			}
			gui->bm_wprintwC(" %s\n", toStr(element.second.readErrors, 9).c_str(), 0);
			gui->bm_wattroffC(4);
			
			++lineCount;
		}
	}
	gui->bm_wattroffC(14);
	
	gui->bm_wmoveC(lineCount, 1);
	gui->bm_wprintwC("Press 'f' to clear data.");

	gui->cropCorruptedIfNeeded(lineCount);
}