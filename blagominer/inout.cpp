#include "inout.h"

#include <algorithm>
#include <sstream>

const std::string header = "File name                                             +DLs      -DLs       I/O";

IUserInterface::~IUserInterface() {}

Output_Curses::~Output_Curses()
{
	bm_end();
}

Output_Curses::Output_Curses(short x, short y, bool lock)
{
	setupSize(x, y, lock);
	bm_init();
}

// TODO: consider variadic template like logger.h:Log()
void Output_Curses::printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
	bool trailingNewLine, bool fillLine, const wchar_t * format, ...)
{
	std::wstring message;

	// TODO: extract, deduplicate
	SYSTEMTIME cur_time;
	GetLocalTime(&cur_time);

	std::wstring time = (
		std::wostringstream()
		<< std::setw(2) << std::setfill(L'0') << cur_time.wHour
		<< ':'
		<< std::setw(2) << std::setfill(L'0') << cur_time.wMinute
		<< ':'
		<< std::setw(2) << std::setfill(L'0') << cur_time.wSecond
		).str();

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

	if (printTimestamp) {

		message = time;
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
			leadingNewLine, trailingNewLine,
			fillLine,
			message });
	}
	{
		std::lock_guard<std::mutex> lockGuard(mLog);
		loggingQueue.push_back(timeMil + L" " + std::wstring(buf.get(), buf.get() + size - 1));
	}
}

void Output_Curses::printHeadlineTitle(
	std::wstring const& appname,
	std::wstring const& version,
	bool elevated
)
{
	printToConsole(12, false, false, true, false, L"%s, %s %s",
		appname.c_str(), version.c_str(),
		elevated ? L"(elevated)" : L"(nonelevated)");
}

void Output_Curses::printWallOfCredits(
	std::vector<std::wstring> const& history
)
{
	for (auto&& item : history)
		printToConsole(4, false, false, true, false, item.c_str());
}

void Output_Curses::printHWInfo(
	hwinfo const& info
)
{
	printToConsole(-1, false, true, false, false, L"CPU support: ");
	if (info.AES)		printToConsole(-1, false, false, false, false, L" AES ");
	if (info.SSE)		printToConsole(-1, false, false, false, false, L" SSE ");
	if (info.SSE2)		printToConsole(-1, false, false, false, false, L" SSE2 ");
	if (info.SSE3)		printToConsole(-1, false, false, false, false, L" SSE3 ");
	if (info.SSE42)		printToConsole(-1, false, false, false, false, L" SSE4.2 ");
	if (info.AVX)		printToConsole(-1, false, false, false, false, L" AVX ");
	if (info.AVX2)		printToConsole(-1, false, false, false, false, L" AVX2 ");
	if (info.AVX512F)	printToConsole(-1, false, false, false, false, L" AVX512F ");

	if (info.avxsupported)	printToConsole(-1, false, false, false, false, L"     [recomend use AVX]");
	if (info.AVX2)			printToConsole(-1, false, false, false, false, L"     [recomend use AVX2]");
	if (info.AVX512F)		printToConsole(-1, false, false, false, false, L"     [recomend use AVX512F]");

	printToConsole(-1, false, true, false, false, L"%S %S [%u cores]", info.vendor.c_str(), info.brand.c_str(), info.cores);
	printToConsole(-1, false, true, true, false, L"RAM: %llu Mb", info.memory);
}

void Output_Curses::printPlotsStart()
{
	printToConsole(15, false, false, true, false, L"Using plots:");
}

void Output_Curses::printPlotsInfo(std::wstring const& directory, size_t nfiles, unsigned long long size)
{
	printToConsole(-1, false, false, true, false, L"%s\tfiles: %4llu\t size: %7llu GiB",
		directory.c_str(), nfiles, size / 1024 / 1024 / 1024);
}

void Output_Curses::printPlotsEnd(unsigned long long total_size)
{
	printToConsole(15, false, false, true, false, L"TOTAL: %llu GiB (%llu TiB)",
		total_size / 1024 / 1024 / 1024, total_size / 1024 / 1024 / 1024 / 1024);
}

void Output_Curses::printThreadActivity(
	std::wstring const& coinName,
	std::wstring const& threadKind,
	std::wstring const& threadAction
)
{
	printToConsole(25, false, false, false, true, L"%s %s thread %s", coinName.c_str(), threadKind.c_str(), threadAction.c_str());
}

void Output_Curses::debugWorkerStats(
	std::wstring const& specialReadMode,
	std::wstring const& directory,
	double proc_time, double work_time,
	unsigned long long files_size_per_thread
)
{
	auto msgFormat = !specialReadMode.empty()
		? L"Thread \"%s\" @ %.1f sec (%.1f MB/s) CPU %.2f%% (%s)"
		: L"Thread \"%s\" @ %.1f sec (%.1f MB/s) CPU %.2f%%%s"; // note that the last %s is always empty, it is there just to keep the same number of placeholders

	printToConsole(7, true, false, true, false, msgFormat,
		directory.c_str(), work_time, (double)(files_size_per_thread) / work_time / 1024 / 1024 / 4096,
		proc_time / work_time * 100);
}

void Output_Curses::printWorkerDeadlineFound(
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long deadline
)
{
	printToConsole(2, true, false, true, false, L"[%20llu|%-10s|Worker] DL found     : %11llu",
		account_id, coinName.c_str(),
		deadline);
}

void Output_Curses::printNetworkHostResolution(
	std::wstring const& lookupWhat,
	std::wstring const& coinName,
	std::string const& remoteName,
	std::vector<char> const& resolvedIP,
	std::string const& remotePost,
	std::string const& remotePath
)
{
	printToConsole(-1, false, false, true, false, L"%s %15s %S (ip %S:%S) %S", coinName.c_str(), lookupWhat.c_str(),
		remoteName.c_str(), resolvedIP.data(), remotePost.c_str(), ((remotePath.length() ? "on /" : "") + remotePath).c_str());
}

void Output_Curses::printNetworkProxyDeadlineReceived(
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long deadline,
	char const (&clientAddr)[22]
)
{
	printToConsole(2, true, false, true, false, L"[%20llu|%-10s|Proxy ] DL found     : %11llu {%S}",
		account_id, coinName.c_str(),
		deadline, clientAddr);
}

void Output_Curses::debugNetworkProxyDeadlineAcked(
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long deadline,
	char const (&clientAddr)[22]
)
{
	printToConsole(9, true, false, true, false, L"[%20llu|%-10s|Proxy ] DL ack'ed    : %11llu {%S}",
		account_id, coinName.c_str(),
		deadline, clientAddr);
}

void Output_Curses::debugNetworkDeadlineDiscarded(
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long deadline,
	unsigned long long targetDeadline
)
{
	printToConsole(2, true, false, true, false, L"[%20llu|%-10s|Sender] DL discarded : %11llu > %11llu",
		account_id, coinName.c_str(),
		deadline, targetDeadline);
}

void Output_Curses::printNetworkDeadlineSent(
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long deadline
)
{
	unsigned long long days = (deadline) / (24 * 60 * 60);
	unsigned hours = (deadline % (24 * 60 * 60)) / (60 * 60);
	unsigned min = (deadline % (60 * 60)) / 60;
	unsigned sec = deadline % 60;

	printToConsole(10, true, false, true, false, L"[%20llu|%-10s|Sender] DL sent      : %11llu %7ud %02u:%02u:%02u",
		account_id, coinName.c_str(),
		deadline,
		days, hours, min, sec);
}

void Output_Curses::printNetworkDeadlineConfirmed(
	bool with_timespan,
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long deadline
)
{
	if (!with_timespan)
	{
		printToConsole(10, true, false, true, false, L"[%20llu|%-10s|Sender] DL confirmed : %s",
			account_id, coinName.c_str(),
			deadline
		);
	}
	else
	{
		unsigned long long days = (deadline) / (24 * 60 * 60);
		unsigned hours = (deadline % (24 * 60 * 60)) / (60 * 60);
		unsigned min = (deadline % (60 * 60)) / 60;
		unsigned sec = deadline % 60;

		printToConsole(10, true, false, true, false, L"[%20llu|%-10s|Sender] DL confirmed : %11llu %7ud %02u:%02u:%02u",
			account_id, coinName.c_str(),
			deadline,
			days, hours, min, sec);
	}
}

void Output_Curses::debugNetworkTargetDeadlineUpdated(
	unsigned long long account_id,
	std::wstring const& coinName,
	unsigned long long targetDeadline
)
{
	printToConsole(10, true, false, true, false, L"[%20llu|%-10s|Sender] Set target DL: %11llu",
		account_id, coinName.c_str(),
		targetDeadline);
}

void Output_Curses::debugRoundTime(
	double threads_time
)
{
	printToConsole(7, true, false, true, false, L"Total round time: %.1f sec", threads_time);
}

void Output_Curses::printBlockEnqueued(
	unsigned long long currentHeight,
	std::wstring const& coinName,
	bool atEnd, bool noQueue
)
{
	if (noQueue)
		printToConsole(5, true, false, false, true, L"[#%7llu|%-10s|Info    ] New block.",
			currentHeight, coinName.c_str());
	else
		printToConsole(5, true, false, false, true, L"[#%7llu|%-10s|Info    ] New block has been added to the%s queue.",
			currentHeight, coinName.c_str(), atEnd ? L" end of the" : L"");
}

void Output_Curses::printRoundInterrupt(
	unsigned long long currentHeight,
	std::wstring const& coinName
)
{
	printToConsole(8, true, false, false, true, L"[#%7llu|%-10s|Info    ] Mining has been interrupted by another coin.",
		currentHeight, coinName.c_str());
}

void Output_Curses::printRoundChangeInfo(bool isResumingInterrupted,
	unsigned long long currentHeight,
	std::wstring const& coinName,
	unsigned long long currentBaseTarget,
	unsigned long long currentNetDiff,
	bool isPoc2Round
)
{
	auto msgFormat = isResumingInterrupted
		? L"[#%7llu|%-10s|Continue] Base Target %7llu %c Net Diff %8llu TiB %c PoC%i"
		: L"[#%7llu|%-10s|Start   ] Base Target %7llu %c Net Diff %8llu TiB %c PoC%i";

	auto colorPair = isResumingInterrupted
		? 5
		: 25;

	printToConsole(colorPair, true, true, false, true, msgFormat,
		currentHeight, coinName.c_str(),
		currentBaseTarget, sepChar,
		currentNetDiff, sepChar,
		isPoc2Round ? 2 : 1);
}

void Output_Curses::printConnQuality(size_t ncoins, std::wstring const& connQualInfo)
{
	if (ncoins != prevNCoins94)
	{
		std::lock_guard<std::mutex> lockGuard(mProgressQueue);
		if (ncoins != prevNCoins94)
			leadingSpace94 = IUserInterface::make_leftpad_for_networkstats(94, ncoins);
	}

	size_t size = swprintf(nullptr, 0, L"%s%s", leadingSpace94.c_str(), connQualInfo.c_str()) + 1llu;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, L"%s%s", leadingSpace94.c_str(), connQualInfo.c_str());

	auto message = std::wstring(buf.get(), buf.get() + size - 1);
	{
		std::lock_guard<std::mutex> lockGuard(mProgressQueue);
		progressQueue.push_back(message);
	}
}

void Output_Curses::printScanProgress(size_t ncoins, std::wstring const& connQualInfo,
	unsigned long long bytesRead, unsigned long long round_size,
	double thread_time, double threads_speed,
	unsigned long long deadline
)
{
	if (ncoins != prevNCoins21)
	{
		std::lock_guard<std::mutex> lockGuard(mProgressQueue);
		if (ncoins != prevNCoins21)
			leadingSpace21 = IUserInterface::make_leftpad_for_networkstats(21, ncoins);
	}

	size_t size = swprintf(nullptr, 0, L"%3llu%% %c %11.2f TiB %c %4.0f s %c %6.0f MiB/s %c Deadline: %s %c %s%s",
		(bytesRead * 4096 * 100 / std::max(1ull, round_size)), sepChar,
		(((double)bytesRead) / (256llu * 1024 * 1024)), sepChar,
		thread_time, sepChar,
		threads_speed, sepChar,
		(deadline == 0) ? L"          -" : toWStr(deadline, 11).c_str(), sepChar,
		leadingSpace94.c_str(), connQualInfo.c_str()) + 1llu;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, L"%3llu%% %c %11.2f TiB %c %4.0f s %c %6.0f MiB/s %c Deadline: %s %c %s%s",
		(bytesRead * 4096 * 100 / std::max(1ull, round_size)), sepChar,
		(((double)bytesRead) / (256llu * 1024 * 1024)), sepChar,
		thread_time, sepChar,
		threads_speed, sepChar,
		(deadline == 0) ? L"          -" : toWStr(deadline, 11).c_str(), sepChar,
		leadingSpace21.c_str(), connQualInfo.c_str());

	auto message = std::wstring(buf.get(), buf.get() + size - 1);
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
				if (consoleOutput.trailingNewLine) {
					waddwstr(win_main, L"\n");
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

void Output_Curses::printFileStats(std::map<std::wstring, t_file_stats>const& fileStats) {
	std::lock_guard lockGuardConsoleWindow(mConsoleWindow);
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
		return;
	}
	else if (lineCount == 0) {
		return;
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
			// TODO: check if bm_wprintwC really supports %S wide char string
			bm_wprintwC("%S %s", toWStr(element.first, 46).c_str(), toStr(element.second.matchingDeadlines, 11).c_str(), 0);
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
	return;
}
