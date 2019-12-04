#include "filemonitor.h"

std::mutex mFileStats;

std::map<std::string, t_file_stats> fileStats = std::map<std::string, t_file_stats>();
bool showCorruptedPlotFiles = true;

bool statsChanged = false;

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

	gui->printFileStats(fileStats);

	statsChanged = false;
}
