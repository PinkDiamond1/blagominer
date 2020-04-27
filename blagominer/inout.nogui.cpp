#include "inout.nogui.h"

const std::string header = "File name                                             +DLs      -DLs       I/O";

Output_PlainText::~Output_PlainText()
{
	interruptConsoleWriter = true;
	if (consoleWriter.joinable())
	{
		consoleWriter.join();
	}
}

Output_PlainText::Output_PlainText()
{
	consoleWriter = std::thread(&Output_PlainText::_consoleWriter, this);
}

void Output_PlainText::printToConsole(
	int colorPair, bool printTimestamp, bool leadingNewLine,
	bool trailingNewLine, bool fillLine, const wchar_t * format,
	...
)
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
			//colorPair,
			leadingNewLine, trailingNewLine,
			fillLine,
			message });
	}
	{
		std::lock_guard<std::mutex> lockGuard(mLog);
		loggingQueue.push_back(timeMil + L" " + std::wstring(buf.get(), buf.get() + size - 1));
	}
}

// TODO: this proc is called VERY often; for now, we want to skip that
// in future it should emit messages via printToConsole, but it should be
// throttled in some way
// also, to be able to guarantee that end-of-round 100% will be displayed
// the parameters must be changed from format+... to specific data
void Output_PlainText::printConnQuality(int ncoins, std::wstring const& connQualInfo)
{
	if (ncoins != prevNCoins94)
	{
		std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
		if (ncoins != prevNCoins94)
			leadingSpace94 = IUserInterface::make_leftpad_for_networkstats(94, ncoins);
	}

	size_t size = swprintf(nullptr, 0, L"%s%s", leadingSpace94.c_str(), connQualInfo.c_str()) + 1;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, L"%s%s", leadingSpace94.c_str(), connQualInfo.c_str());

	auto message = std::wstring(buf.get(), buf.get() + size - 1);
	{
		std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
		consoleQueue.push_back({
			false, true,
			false,
			message });
	}
}

void Output_PlainText::printScanProgress(int ncoins, std::wstring const& connQualInfo,
	unsigned long long bytesRead, unsigned long long round_size,
	double thread_time, double threads_speed,
	unsigned long long deadline
)
{
	if (ncoins != prevNCoins21)
	{
		std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
		if (ncoins != prevNCoins21)
			leadingSpace21 = IUserInterface::make_leftpad_for_networkstats(21, ncoins);
	}

	size_t size = swprintf(nullptr, 0, L"%3llu%% %c %11.2f TiB %c %4.0f s %c %6.0f MiB/s %c Deadline: %s %c %s%s",
		(bytesRead * 4096 * 100 / round_size), sepChar,
		(((double)bytesRead) / (256 * 1024 * 1024)), sepChar,
		thread_time, sepChar,
		threads_speed, sepChar,
		(deadline == 0) ? L"          -" : toWStr(deadline, 11).c_str(), sepChar,
		leadingSpace94.c_str(), connQualInfo.c_str()) + 1;
	std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	swprintf(buf.get(), size, L"%3llu%% %c %11.2f TiB %c %4.0f s %c %6.0f MiB/s %c Deadline: %s %c %s%s",
		(bytesRead * 4096 * 100 / round_size), sepChar,
		(((double)bytesRead) / (256 * 1024 * 1024)), sepChar,
		thread_time, sepChar,
		threads_speed, sepChar,
		(deadline == 0) ? L"          -" : toWStr(deadline, 11).c_str(), sepChar,
		leadingSpace21.c_str(), connQualInfo.c_str());

	auto message = std::wstring(buf.get(), buf.get() + size - 1);
	{
		std::lock_guard<std::mutex> lockGuard(mConsoleQueue);
		consoleQueue.push_back({
			false, true,
			false,
			message });
	}
}

void Output_PlainText::_consoleWriter() {
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
			
			if (consoleOutput.leadingNewLine) {
				wprintf(L"\n");
			}
			wprintf(consoleOutput.message.c_str());
			if (consoleOutput.trailingNewLine) {
				wprintf(L"\n");
			}
		}
		else {
			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

bool Output_PlainText::currentlyDisplayingCorruptedPlotFiles() {
	return false;
}

int Output_PlainText::bm_wgetchMain() {
	// TODO: maybe somehow read nonblocking lines from CIN?
	// are we even interested in receiving any control commands other than KILL?
	return -1;
}

void Output_PlainText::showNewVersion(std::string version) {
	std::string tmp = "New version available: " + version + "\n";
	printToConsole(0, false, true, true, false, std::wstring(tmp.begin(), tmp.end()).c_str());
}

void Output_PlainText::printFileStats(std::map<std::string, t_file_stats>const& fileStats) {
	int lineCount = 0;
	for (auto& element : fileStats)
		if (element.second.conflictingDeadlines > 0 || element.second.readErrors > 0)
			++lineCount;

	if (lineCount == 0)
		return;

	std::string tmp = header;
	tmp += "\n";

	for (auto& element : fileStats)
		if (element.second.conflictingDeadlines > 0 || element.second.readErrors > 0) {
			tmp += toStr(element.first, 46);
			tmp += " ";
			tmp += toStr(element.second.matchingDeadlines, 11);
			//if (element.second.conflictingDeadlines > 0) {
			//	bm_wattronC(4);
			//}
			tmp += " ";
			tmp += toStr(element.second.conflictingDeadlines, 9);
			//if (element.second.readErrors > 0) {
			//	bm_wattronC(4);
			//}
			tmp += " ";
			tmp += toStr(element.second.readErrors, 9);
			tmp += "\n";
		}

	printToConsole(0, false, true, true, false, std::wstring(tmp.begin(), tmp.end()).c_str());
}
