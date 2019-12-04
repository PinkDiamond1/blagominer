#include "stdafx.h"
#include "inout.h"

IOutput_Curses::~IOutput_Curses() {}

Output_Curses::~Output_Curses()
{
	bm_end();
}

Output_Curses::Output_Curses(short x, short y, bool lock)
{
	setupSize(x, y, lock);
	bm_init();
}

std::unique_lock<std::mutex> Output_Curses::lock_outputDevice()
{
	return std::unique_lock(mConsoleWindow);
}

void Output_Curses::printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
	bool trailingNewLine, bool fillLine, const wchar_t * format, ...)
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

	va_list args;
	va_start(args, format);
	int size = vswprintf(nullptr, 0, format, args) + 1;
	va_end(args);
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	va_list args2;
	va_start(args2, format);
	vswprintf(buf.get(), size, format, args2);
	va_end(args2);
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
}

void Output_Curses::printToProgress(const wchar_t * format, ...)
{
	std::wstring message;

	va_list args;
	va_start(args, format);
	size_t size = vswprintf(nullptr, 0, format, args) + 1;
	va_end(args);
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	va_list args2;
	va_start(args2, format);
	vswprintf(buf.get(), size, format, args2);
	va_end(args2);
	message += std::wstring(buf.get(), buf.get() + size - 1);
	{
		std::lock_guard<std::mutex> lockGuard(mProgressQueue);
		progressQueue.push_back(message);
	}
}

void Output_Curses::_progressWriter() {
	while (!interruptConsoleWriter) {
		if (!progressQueue.empty()) {

			std::wstring message;

			{
				std::lock_guard<std::mutex> lockGuard(mProgressQueue);
				message = progressQueue.front();
				progressQueue.pop_front();
			}

			wclear(win_progress);
			wmove(win_progress, 1, 1);
			box(win_progress, 0, 0);
			wattron(win_progress, COLOR_PAIR(14));
			waddwstr(win_progress, message.c_str());
			wattroff(win_progress, COLOR_PAIR(14));
			wrefresh(win_progress);
		}
		else {
			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

void Output_Curses::_consoleWriter() {
	while (!interruptConsoleWriter) {
		if (!consoleQueue.empty()) {
			ConsoleOutput consoleOutput;
			bool skip = false;
			
			{
				std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
				if (consoleQueue.size() == 1 && consoleQueue.front().message == L"\n") {
					skip = true;
				}
			}

			if (skip) {
				std::this_thread::yield();
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			} 
			else {
				std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
				consoleOutput = consoleQueue.front();
				consoleQueue.pop_front();
			}
			
			{
				std::lock_guard<std::mutex> lockGuard(mConsoleWindow);
				
				if (consoleOutput.colorPair >= 0) {
					wattron(win_main, COLOR_PAIR(consoleOutput.colorPair));
				}
				if (consoleOutput.leadingNewLine) {
					waddwstr(win_main, L"\n");
				}
				waddwstr(win_main, consoleOutput.message.c_str());
				if (consoleOutput.fillLine) {
					int y;
					int x;
					getyx(win_main, y, x);
					const int remaining = COLS - x;

					if (remaining > 0) {
						waddwstr(win_main, std::wstring(remaining, ' ').c_str());
						int newY;
						int newX;
						getyx(win_main, newY, newX);
						if (newX != 0) {
							waddwstr(win_main, std::wstring(L"\n").c_str());
						}
					}
				}
				if (consoleOutput.colorPair >= 0) {
					wattroff(win_main, COLOR_PAIR(consoleOutput.colorPair));
				}
				wrefresh(win_main);
			}
			
		}
		else {
			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

bool Output_Curses::currentlyDisplayingCorruptedPlotFiles() {
	return getbegy(win_corrupted) >= 0;
}

bool Output_Curses::currentlyDisplayingNewVersion() {
	return getbegy(win_new_version) >= 0;
}

int Output_Curses::bm_wgetchMain() {
	return wgetch(win_main);
}

//Turn on color attribute
int Output_Curses::bm_wattronC(int color) {
	return wattron(win_corrupted, COLOR_PAIR(color));
}

//Turn off color attribute
int Output_Curses::bm_wattroffC(int color) {
	return wattroff(win_corrupted, COLOR_PAIR(color));
}

int Output_Curses::bm_wprintwC(const char * output, ...) {
	va_list args;
	va_start(args, output);
	return vw_printw(win_corrupted, output, args);
	va_end(args);
}

void Output_Curses::setupSize(short& x, short& y, bool& lock)
{
	if (x < 96) x = 96;
	if (y < 20) y = 20;
	win_size_x = x;
	win_size_y = y;
	lockWindowSize = lock;
}

static void handleReturn(BOOL success) {
	if (!success) {
		Log(L"FAILED with error %lu", GetLastError());
	}
}

static void resizeConsole(SHORT newColumns, SHORT newRows, BOOL lockWindowSize) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi; // Hold Current Console Buffer Info 
	BOOL bSuccess;
	SMALL_RECT newWindowRect;         // Hold the New Console Size 
	COORD currentWindowSize;

	Log(L"GetConsoleScreenBufferInfo");
	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	handleReturn(bSuccess);
	currentWindowSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	currentWindowSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	Log(L"Current buffer size csbi.dwSize X: %hi, Y: %hi", csbi.dwSize.X, csbi.dwSize.Y);
	Log(L"csbi.dwMaximumWindowSize X: %hi, Y: %hi", csbi.dwMaximumWindowSize.X, csbi.dwMaximumWindowSize.Y);
	Log(L"currentWindowSize X: %hi, Y: %hi", currentWindowSize.X, currentWindowSize.Y);

	// Get the Largest Size we can size the Console Window to 
	COORD largestWindowSize = GetLargestConsoleWindowSize(hConsole);
	Log(L"largestWindowSize X: %hi, Y: %hi", largestWindowSize.X, largestWindowSize.Y);

	// Define the New Console Window Size and Scroll Position 
	newWindowRect.Right = min(newColumns, largestWindowSize.X) - 1;
	newWindowRect.Bottom = min(newRows, largestWindowSize.Y) - 1;
	newWindowRect.Left = newWindowRect.Top = (SHORT)0;

	Log(L"newWindowRect b: %hi, l: %hi, r: %hi, t: %hi", newWindowRect.Bottom, newWindowRect.Left, newWindowRect.Right, newWindowRect.Top);

	// Define the New Console Buffer Size
	COORD newBufferSize;
	newBufferSize.X = min(newColumns, largestWindowSize.X);
	newBufferSize.Y = min(newRows, largestWindowSize.Y);

	Log(L"Resizing buffer (x: %hi, y: %hi).", newBufferSize.X, newBufferSize.Y);
	Log(L"Resizing window (x: %hi, y: %hi).", newWindowRect.Right - newWindowRect.Left, newWindowRect.Bottom - newWindowRect.Top);


	/*
		Information from https://docs.microsoft.com/en-us/windows/console/window-and-screen-buffer-size

		To change a screen buffer's size, use the SetConsoleScreenBufferSize function. This function
		fails if either dimension of the specified size is less than the corresponding dimension of the
		console's window.

		To change the size or location of a screen buffer's window, use the SetConsoleWindowInfo function.
		This function fails if the specified window-corner coordinates exceed the limits of the console
		screen buffer or the screen. Changing the window size of the active screen buffer changes the
		size of the console window displayed on the screen.

	*/
	while (true) {
		if (currentWindowSize.X > newBufferSize.X || currentWindowSize.Y > newBufferSize.Y) {
			Log(L"Current window size is larger than the new buffer size. Resizing window first.");
			Log(L"SetConsoleWindowInfo srWindowRect b: %hi, l: %hi, r: %hi, t: %hi", newWindowRect.Bottom, newWindowRect.Left, newWindowRect.Right, newWindowRect.Top);
			bSuccess = SetConsoleWindowInfo(hConsole, TRUE, &newWindowRect);
			handleReturn(bSuccess);
			//std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
			Log(L"SetConsoleScreenBufferSize coordScreen X: %hi, Y: %hi", newBufferSize.X, newBufferSize.Y);
			bSuccess = SetConsoleScreenBufferSize(hConsole, newBufferSize);
			handleReturn(bSuccess);
		}
		else {
			Log(L"SetConsoleScreenBufferSize coordScreen X: %hi, Y: %hi", newBufferSize.X, newBufferSize.Y);
			bSuccess = SetConsoleScreenBufferSize(hConsole, newBufferSize);
			handleReturn(bSuccess);
			//std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
			Log(L"SetConsoleWindowInfo srWindowRect b: %hi, l: %hi, r: %hi, t: %hi", newWindowRect.Bottom, newWindowRect.Left, newWindowRect.Right, newWindowRect.Top);
			bSuccess = SetConsoleWindowInfo(hConsole, TRUE, &newWindowRect);
			handleReturn(bSuccess);
		}

		HWND consoleWindow = GetConsoleWindow();

		// Get the monitor that is displaying the window
		HMONITOR monitor = MonitorFromWindow(consoleWindow, MONITOR_DEFAULTTONEAREST);

		// Get the monitor's offset in virtual-screen coordinates
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfoA(monitor, &monitorInfo);

		RECT wSize;
		GetWindowRect(consoleWindow, &wSize);
		Log(L"Window Rect wSize b: %hi, l: %hi, r: %hi, t: %hi", wSize.bottom, wSize.left, wSize.right, wSize.top);
		// Move window to top
		Log(L"MoveWindow X: %ld, Y: %ld, w: %ld, h: %ld", wSize.left, monitorInfo.rcWork.top, wSize.right - wSize.left, wSize.bottom - wSize.top);
		bSuccess = MoveWindow(consoleWindow, wSize.left, monitorInfo.rcWork.top, wSize.right - wSize.left, wSize.bottom - wSize.top, true);
		handleReturn(bSuccess);

		if (lockWindowSize) {
			//Prevent resizing. Source: https://stackoverflow.com/a/47359526
			SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
		}

		Log(L"GetConsoleScreenBufferInfo");
		bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
		handleReturn(bSuccess);
		currentWindowSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		currentWindowSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		Log(L"New buffer size csbi.dwSize X: %hi, Y: %hi", csbi.dwSize.X, csbi.dwSize.Y);
		Log(L"New window size X: %hi, Y: %hi", currentWindowSize.X, currentWindowSize.Y);

		Log(L"Hiding scroll bars.");
		bSuccess = ShowScrollBar(consoleWindow, SB_BOTH, FALSE);
		handleReturn(bSuccess);

		if (currentWindowSize.X != newBufferSize.X || currentWindowSize.Y != newBufferSize.Y) {
			Log(L"Failed to resize window. Retrying.");
		}
		else {
			break;
		}

	}

	return;
}

// init screen
void Output_Curses::bm_init() {
	resizeConsole(win_size_x, win_size_y, lockWindowSize);

	initscr();
	raw();
	cbreak();		// don't use buffer for getch()
	noecho();		// do not display keystrokes
	curs_set(0);	// remove the cursor
	start_color();	// it will be all colorful			

	//int init_pair(short pair, short foreground, short background);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLACK);
	init_pair(5, COLOR_BLACK, COLOR_WHITE);
	init_pair(6, COLOR_CYAN, COLOR_BLACK);
	init_pair(7, COLOR_WHITE, COLOR_BLACK);
	init_pair(8, COLOR_BLACK, COLOR_YELLOW);
	init_pair(9, 9, COLOR_BLACK);
	init_pair(10, 10, COLOR_BLACK);
	init_pair(11, 11, COLOR_BLACK);
	init_pair(12, 12, COLOR_BLACK);
	init_pair(14, 14, COLOR_BLACK);
	init_pair(15, 15, COLOR_BLACK);
	init_pair(25, 15, COLOR_BLUE);

	win_main = newwin(LINES - progress_lines, COLS, 0, 0);
	scrollok(win_main, true);
	keypad(win_main, true);
	nodelay(win_main, true);
	win_progress = newwin(progress_lines, COLS, LINES - progress_lines, 0);
	leaveok(win_progress, true);
	win_corrupted = newwin(1, COLS, -1, 0);
	leaveok(win_corrupted, true);
	win_new_version = newwin(new_version_lines, COLS, -1, 0);
	leaveok(win_corrupted, true);

	consoleWriter = std::thread(&Output_Curses::_consoleWriter, this);
	progressWriter = std::thread(&Output_Curses::_progressWriter, this);
}

void Output_Curses::bm_end() {
	interruptConsoleWriter = true;
	if (consoleWriter.joinable())
	{
		consoleWriter.join();
	}
	if (progressWriter.joinable())
	{
		progressWriter.join();
	}
}

void Output_Curses::refreshCorrupted() {
	if (currentlyDisplayingCorruptedPlotFiles()) {
		wrefresh(win_corrupted);
	}
}

void Output_Curses::showNewVersion(std::string version) {
	std::lock_guard<std::mutex> lockGuardConsoleWindow(mConsoleWindow);
	version = "New version available: " + version;
	if (!currentlyDisplayingNewVersion()) {
		mvwin(win_new_version, 0, 0);
		int winMainY = getbegy(win_main);
		int winMainRow = getmaxy(win_main);

		int winMainOffset = winMainY + new_version_lines;
		winMainRow -= new_version_lines;

		if (currentlyDisplayingCorruptedPlotFiles()) {
			int totalSpaceNeeded = new_version_lines + getRowsCorrupted() + progress_lines + minimumWinMainHeight;
			if (totalSpaceNeeded > LINES) {
				Log(L"Terminal too small to output everything (%i).", totalSpaceNeeded);
				int corruptedLineCount = LINES - new_version_lines - progress_lines - minimumWinMainHeight;
				Log(L"Setting corrupted linecount to %i", corruptedLineCount);
				resizeCorrupted(corruptedLineCount);
				cropCorruptedIfNeeded(corruptedLineCount+1);
			}
			else {
				wresize(win_main, winMainRow, COLS);
				mvwin(win_main, winMainOffset, 0);
			}
			mvwin(win_corrupted, new_version_lines, 0);
		}
		else {
			wresize(win_main, winMainRow, COLS);
			mvwin(win_main, winMainOffset, 0);
		}
	}

	clearNewVersion();
	wattron(win_new_version, COLOR_PAIR(4));
	box(win_new_version, 0, 0);
	wattroff(win_new_version, COLOR_PAIR(4));
	wmove(win_new_version, 1, 1);
	wattron(win_new_version, COLOR_PAIR(14));
	waddstr(win_new_version, version.c_str());
	wattroff(win_new_version, COLOR_PAIR(14));

	wrefresh(win_main);
	refreshCorrupted();
	wrefresh(win_new_version);
}

void Output_Curses::cropCorruptedIfNeeded(int lineCount) {
	int rowsCorrupted = getRowsCorrupted();
	if (rowsCorrupted < lineCount) {
		bm_wmoveC(rowsCorrupted - 3, 1);
		clearCorruptedLine();
		bm_wprintwC("Not enough space to display all data.");
		bm_wmoveC(rowsCorrupted - 2, 1);
		clearCorruptedLine();
		bm_wprintwC("Press 'f' to clear data.");
	}
	boxCorrupted();
	refreshCorrupted();
}

void Output_Curses::resizeCorrupted(int lineCount) {
	int winVerRow = 0;
	if (currentlyDisplayingNewVersion()) {
		winVerRow = getmaxy(win_new_version);
	}
		
	if (lineCount > 0) {
		if (!currentlyDisplayingCorruptedPlotFiles()) {
			mvwin(win_corrupted, winVerRow, 0);
		}
		
		int totalSpaceNeeded = winVerRow + lineCount + minimumWinMainHeight + progress_lines;
		if (totalSpaceNeeded > LINES) {
			Log(L"Terminal too small to output everything (lineCount: %i).", lineCount);
			lineCount = LINES - winVerRow - minimumWinMainHeight - progress_lines;
			Log(L"Setting linecount to %i", lineCount);
		}
		
		wresize(win_main, LINES - winVerRow - lineCount - progress_lines, COLS);
		mvwin(win_main, lineCount + winVerRow, 0);
		wresize(win_corrupted, lineCount, COLS);
	}
	else if (lineCount == 0) {
		mvwin(win_main, winVerRow, 0);
		wresize(win_main, LINES - winVerRow - progress_lines, COLS);
	}
}

int Output_Curses::getRowsCorrupted() {
	return getmaxy(win_corrupted);
}

void Output_Curses::clearCorrupted() {
	if (currentlyDisplayingCorruptedPlotFiles()) {
		mvwin(win_corrupted, -1, 0);
		wclear(win_corrupted);
	}
}
void Output_Curses::clearCorruptedLine() {
	wclrtoeol(win_corrupted);
}
void Output_Curses::clearNewVersion() {
	wclear(win_new_version);
}

void Output_Curses::hideCorrupted() {
	if (currentlyDisplayingCorruptedPlotFiles()) {
		win_corrupted = newwin(1, COLS, -1, 0);
		leaveok(win_corrupted, true);
	}
}

int Output_Curses::bm_wmoveC(int line, int column) {
	return wmove(win_corrupted, line, column);
};

void Output_Curses::boxCorrupted() {
	wattron(win_corrupted, COLOR_PAIR(4));
	box(win_corrupted, 0, 0);
	wattroff(win_corrupted, COLOR_PAIR(4));
}

int Output_Curses::printFileStats(int oldLineCount, std::string header, std::map<std::string, t_file_stats>const& fileStats) {
	auto lockGuardConsoleWindow(lock_outputDevice());
	int lineCount = 0;
	for (auto& element : fileStats) {
		if (element.second.conflictingDeadlines > 0 || element.second.readErrors > 0) {
			++lineCount;
		}
	}

	if (lineCount == 0 && currentlyDisplayingCorruptedPlotFiles()) {
		hideCorrupted();
		resizeCorrupted(0);
		refreshCorrupted();
		return oldLineCount;
	}
	else if (lineCount == 0) {
		return oldLineCount;
	}

	// Increase for header, border and for clear message.
	lineCount += 4;

	if (lineCount != oldLineCount) {
		clearCorrupted();
		resizeCorrupted(lineCount);
		oldLineCount = lineCount;
	}
	refreshCorrupted();

	lineCount = 1;
	bm_wmoveC(lineCount++, 1);
	bm_wprintwC("%s", header.c_str(), 0);

	for (auto& element : fileStats) {
		if (element.second.conflictingDeadlines > 0 || element.second.readErrors > 0) {
			bm_wattronC(14);
			bm_wmoveC(lineCount, 1);
			bm_wprintwC("%s %s", toStr(element.first, 46).c_str(), toStr(element.second.matchingDeadlines, 11).c_str(), 0);
			if (element.second.conflictingDeadlines > 0) {
				bm_wattronC(4);
			}
			bm_wprintwC(" %s", toStr(element.second.conflictingDeadlines, 9).c_str(), 0);
			bm_wattroffC(4);
			bm_wattronC(14);
			if (element.second.readErrors > 0) {
				bm_wattronC(4);
			}
			bm_wprintwC(" %s\n", toStr(element.second.readErrors, 9).c_str(), 0);
			bm_wattroffC(4);

			++lineCount;
		}
	}
	bm_wattroffC(14);

	bm_wmoveC(lineCount, 1);
	bm_wprintwC("Press 'f' to clear data.");

	cropCorruptedIfNeeded(lineCount);
	return oldLineCount;
}

std::wstring make_filled_string(int nspaces, wchar_t filler)
{
	return std::wstring(max(0, nspaces), filler);
}

std::wstring make_leftpad_for_networkstats(int availablespace, int nactivecoins)
{
	const int remainingspace = availablespace - (nactivecoins * 4) - (nactivecoins - 1);
	return make_filled_string(remainingspace, L' ');
}
