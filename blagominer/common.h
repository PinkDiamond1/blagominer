#pragma once
#include "common-pragmas.h"

#include <mutex>
#include <array>
#include <vector>
#include <ctime>
#include <math.h>
#include <optional>
#include <string>
#include <map>
#include <stdexcept>
#include <algorithm>

#include "heapallocator.h"
#include "logger.h"

#include <curl/curl.h>

extern const wchar_t sepChar;

extern double checkForUpdateInterval;
extern bool ignoreSuspectedFastBlocks;
extern volatile bool exit_flag;							// true if miner is to be exited

extern HANDLE hHeap;							//heap
extern heap_allocator<char> theHeap;

enum class MiningState {
	QUEUED,
	MINING,
	DONE,
	INTERRUPTED
};

struct t_shares {
	t_shares(std::wstring const& file_name, unsigned long long account_id, unsigned long long best,
		unsigned long long nonce, unsigned long long deadline, unsigned long long height,
		unsigned long long baseTarget) :
		file_name(file_name), account_id(account_id), best(best), nonce(nonce), deadline(deadline),
		height(height), baseTarget(baseTarget) {}
	std::wstring file_name;
	unsigned long long account_id;// = 0;
	// This is the best Target, not Deadline.
	unsigned long long best;// = 0;
	unsigned long long nonce;// = 0;
	unsigned long long deadline;
	unsigned long long height;
	unsigned long long baseTarget;
};

struct t_best {
	unsigned long long account_id;// = 0;
	unsigned long long best;// = 0;
	unsigned long long nonce;// = 0;
	unsigned long long DL;// = 0;
	unsigned long long targetDeadline;// = 0;
};

struct t_session {
	t_session(SOCKET Socket, unsigned long long deadline, t_shares body) : Socket(Socket), deadline(deadline), body(body) {}
	SOCKET Socket;
	unsigned long long deadline;
	t_shares body;
};
struct t_session2 {
	t_session2(CURL* curl, unsigned long long deadline, t_shares body) : curl(curl), deadline(deadline), body(body) {}
	CURL* curl;
	unsigned long long deadline;
	t_shares body;
};

struct t_files {
	bool done;
	std::wstring Path;
	std::wstring Name;
	long long positionOnNtfsVolume;
	unsigned long long Size;
	unsigned long long Key;
	unsigned long long StartNonce;
	unsigned long long Nonces;
	unsigned long long Stagger;
	unsigned long long Offset;
	bool P2;
	bool BFS;
};

struct t_directory_info {
	t_directory_info(std::wstring dir, bool done, std::vector<t_files> files) : dir(dir), done(done), files(files) {}
	std::wstring dir;
	bool done;
	std::vector<t_files> files;
};

struct t_gui {
	bool disableGui;
	short size_x;
	short size_y;
	bool lockWindowSize;
};

struct t_logging {
	bool enableLogging;
	bool enableCsv;
	bool logAllGetMiningInfos;				// Prevent spamming the log file by only outputting
											// GMI when there is a change in the GMI
};

struct LogFileInfo {
	std::wstring filename;
	std::mutex mutex;
};

struct CoinLogFiles {
	LogFileInfo failed;
	LogFileInfo submitted;
};


// TODO: this locking scheme IS SO WRONG.. and even things like 'baseTarget' or 'scoop' are not protected..
struct t_locks {
	std::mutex mHeight;
	std::mutex mTargetDeadlineInfo;
	std::mutex mSignature;
	std::mutex mStrSignature;
	std::mutex mOldSignature;
	std::mutex mCurrentStrSignature;
	std::mutex mNewMiningInfoReceived;

	std::mutex sessionsLock;		// session lock
	std::mutex sessions2Lock;		// session2 lock
	std::mutex bestsLock;			// best lock
	std::mutex sharesLock;			// shares lock

	volatile bool stopRoundSpecificNetworkingThreads = false;
};

// TODO: instead of all-zero init, provide a ctor: init_mining_info
struct t_mining_info {
	bool enable = false;
	bool newMiningInfoReceived = false;
	size_t miner_mode = 1;					// miner mode. 0=solo, 1=pool
	size_t priority = 0;
	MiningState state = MiningState::DONE;
	unsigned long long baseTarget = 0;			// base target of current block
	unsigned long long targetDeadlineInfo = 0;	// target deadline info from pool
	unsigned long long height = 0;				// current block height
	unsigned long long deadline = 0;			// current deadline
	unsigned long long my_target_deadline = 0;
	unsigned long long POC2StartBlock = 0;
	bool enableDiskcoinGensigs = false;
	unsigned int scoop = 0;						// currenty scoop
	std::vector<std::shared_ptr<t_directory_info>> dirs;

	// Values for current mining process
	std::array<uint8_t, 32> currentSignature;
	std::wstring current_str_signature;
	unsigned long long currentHeight = 0;
	unsigned long long currentBaseTarget = 0;
	std::vector<t_best> bests;
	std::vector<std::shared_ptr<t_shares>> shares;

	// Values for new mining info check
	std::array<uint8_t, 32> signature;			// signature of current block
	std::array<uint8_t, 32> oldSignature;		// signature of last block
	std::wstring str_signature;
};

// TODO: instead of all-zero init, provide a ctor: init_coinNetwork
struct t_network_info {
	std::string nodeaddr;
	std::string nodeport;
	std::string noderoot;
	std::string solopass;
	unsigned submitTimeout = 1000;
	std::string updateraddr;
	std::string updaterport;
	std::string updaterroot;
	bool enable_proxy = false;						// enable client/server functionality
	std::string proxyport;
	size_t send_interval = 100;
	size_t update_interval = 1000;
	size_t proxy_update_interval = 500;
	int network_quality = 0;
	bool usehttps = false;
	std::string sendextraquery;
	std::string sendextraheader;
	std::vector<std::shared_ptr<t_session>> sessions;
	std::vector<std::shared_ptr<t_session2>> sessions2;
	std::thread sender;
	std::thread confirmer;
};

struct t_roundreplay_round;
struct t_roundreplay_round_test;

// TODO: instead of all-zero init, provide a ctor: init_mining_info
struct t_coin_info {
	std::wstring coinname;
	CoinLogFiles logging;
	std::shared_ptr<t_mining_info> mining;
	std::shared_ptr<t_network_info> network;
	std::shared_ptr<t_locks> locks;
	std::thread proxyThread;
	std::thread updaterThread;
	std::thread proxyOnlyThread;

	bool isPoc2Round();

	t_roundreplay_round* testround1 = 0;
	t_roundreplay_round_test* testround2 = 0;
};

struct t_file_stats {
	unsigned long long matchingDeadlines;
	unsigned long long conflictingDeadlines;
	unsigned long long readErrors;
};


extern std::vector<std::shared_ptr<t_coin_info>> allcoins;
extern std::vector<std::shared_ptr<t_coin_info>> coins;
extern t_logging loggingConfig;

struct t_roundreplay_round_test {
	enum class RoundTestMode { RMT_UNKNOWN = 0, RMT_NORMAL = 1, RMT_OFFLINE = 2 };
	RoundTestMode mode = RoundTestMode::RMT_UNKNOWN;

	unsigned long long assume_account = 0;
	unsigned long long assume_nonce = 0;

	std::optional<unsigned int> assume_scoop;
	std::optional<std::wstring> assume_scoop_low;
	std::optional<std::wstring> assume_scoop_high;

	std::optional<unsigned int> check_scoop;
	std::optional<std::wstring> check_scoop_low;
	std::optional<std::wstring> check_scoop_high;
	std::optional<unsigned long long> check_deadline;

	std::optional<bool> passed_scoop;
	std::optional<bool> passed_scoop_low;
	std::optional<bool> passed_scoop_high;
	std::optional<bool> passed_deadline;
};

struct t_roundreplay_round {
	unsigned long long height = 0;
	std::wstring signature;
	unsigned long long baseTarget = 0;

	std::optional<bool> assume_POC2; // TODO: since it's round X test, move that to the TEST // obvious in OFFLINE, and in ONLINE remember the mode is SHORT not FULL!

	std::vector<t_roundreplay_round_test> tests;
};

struct t_roundreplay {
	bool isEnabled = false;
	std::wstring coinName;
	std::vector<t_roundreplay_round> rounds;
};

struct t_testmode_config {
	bool isEnabled = false;
	t_roundreplay roundReplay;
};

extern t_testmode_config testmodeConfig;


unsigned long long getHeight(std::shared_ptr<t_coin_info> coin);
void setHeight(std::shared_ptr<t_coin_info> coin, const unsigned long long height);

unsigned long long getTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin);
void setTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin, const unsigned long long targetDeadlineInfo);
std::array<uint8_t, 32> getSignature(std::shared_ptr<t_coin_info> coin);
std::wstring getCurrentStrSignature(std::shared_ptr<t_coin_info> coin);
void setSignature(std::shared_ptr<t_coin_info> coin, std::array<uint8_t, 32> const& signature);
void setStrSignature(std::shared_ptr<t_coin_info> coin, std::wstring const& signature);
void updateOldSignature(std::shared_ptr<t_coin_info> coin);
void updateCurrentStrSignature(std::shared_ptr<t_coin_info> coin);
bool signaturesDiffer(std::shared_ptr<t_coin_info> coin);
bool signaturesDiffer(std::shared_ptr<t_coin_info> coin, std::array<uint8_t, 32> const& sig);
bool haveReceivedNewMiningInfo(const std::vector<std::shared_ptr<t_coin_info>>& coins);
void setnewMiningInfoReceived(std::shared_ptr<t_coin_info> coin, const bool val);
int getNetworkQuality(std::shared_ptr<t_coin_info> coin);

void getLocalDateTime(const std::time_t &rawtime, char* local, const std::string sepTime = ":");

std::wstring toWStr(int number, const unsigned short length);
std::wstring toWStr(unsigned long long number, const unsigned short length);
std::wstring toWStr(std::wstring str, const unsigned short length);
std::wstring toWStr(std::string str);

std::string toStr(unsigned long long number, const unsigned short length);
std::string toStr(std::string str, const unsigned short length);
std::string toStr(std::wstring str);

// TODO: find/invent something better for statically detecting narrowing overflows on constexprs
// https://stackoverflow.com/a/46229281/717732
template<class Target, class Source>
Target narrow_cast(Source v)
{
	auto r = static_cast<Target>(v);
	if (static_cast<Source>(r) != v)
		throw std::runtime_error("narrow_cast<>() failed");
	return r;
}

template<typename T>
class span {
	T* _data;
	size_t _size;
public:
	span(T* data, size_t size) :_data(data), _size(size) {}
	T* data() const { return _data; }
	size_t size() const { return _size; }
};

template<unsigned parseFlags, typename T>
DocumentUTF16LE parseJsonData(T const& source)
{
	MemoryStream bis(source.data(), source.size());
	AutoUTFInputStream<unsigned, MemoryStream> eis(bis);

	DocumentUTF16LE document;
	document.ParseStream<parseFlags, AutoUTF<unsigned>>(eis);
	return document;
}

template<unsigned parseFlags, typename T>
DocumentUTF16LE& parseJsonData(DocumentUTF16LE& document, T const& source)
{
	MemoryStream bis(source.data(), source.size());
	AutoUTFInputStream<unsigned, MemoryStream> eis(bis);
	return document.ParseStream<parseFlags, AutoUTF<unsigned>>(eis);
}


struct IUserInterface;

extern std::unique_ptr<IUserInterface> gui;

struct hwinfo
{
	bool AES = false;
	bool SSE = false, SSE2 = false, SSE3 = false, SSE42 = false;
	bool AVX = false, AVX2 = false, AVX512F = false, avxsupported = false;
	std::string vendor, brand;
	unsigned long cores = 0;
	unsigned long long memory = 0; // in megabytes
};

struct IUserInterface
{
	virtual ~IUserInterface() = 0;

	virtual void printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
		bool trailingNewLine, bool fillLine, const wchar_t * format, ...) = 0;

	virtual void printHeadlineTitle(
		std::wstring const& appname,
		std::wstring const& version,
		bool elevated
	) = 0;
	virtual void printWallOfCredits(
		std::vector<std::wstring> const& history
	) = 0;
	virtual void printHWInfo(
		hwinfo const& info
	) = 0;

	virtual void printPlotsStart() = 0;
	virtual void printPlotsInfo(std::wstring const& directory, size_t nfiles, unsigned long long size) = 0;
	virtual void printPlotsEnd(unsigned long long total_size) = 0;

	virtual void printThreadActivity(
		std::wstring const& coinName,
		std::wstring const& threadKind,
		std::wstring const& threadAction
	) = 0;

	virtual void debugWorkerStats(
		std::wstring const& specialReadMode,
		std::wstring const& directory,
		double proc_time, double work_time,
		unsigned long long files_size_per_thread
	) = 0;
	virtual void printWorkerDeadlineFound(
		unsigned long long account_id,
		std::wstring const& coinname,
		unsigned long long deadline
	) = 0;
	virtual void printScanProgress(
		size_t ncoins, std::wstring const& connQualInfo,
		unsigned long long bytesRead, unsigned long long round_size,
		double thread_time, double threads_speed,
		unsigned long long deadline
	) = 0;

	virtual void printNetworkHostResolution(
		std::wstring const& lookupWhat,
		std::wstring const& coinName,
		std::string const& remoteName,
		std::vector<char> const& resolvedIP,
		std::string const& remotePost,
		std::string const& remotePath
	) = 0;
	virtual void printNetworkProxyDeadlineReceived(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		char const (&clientAddr)[22]
	) = 0;
	virtual void debugNetworkProxyDeadlineAcked(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		char const (&clientAddr)[22]
	) = 0;
	virtual void debugNetworkDeadlineDiscarded(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		unsigned long long targetDeadline
	) = 0;
	virtual void printNetworkDeadlineSent(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline
	) = 0;
	virtual void printNetworkDeadlineConfirmed(
		bool with_timespan,
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline
	) = 0;
	virtual void debugNetworkTargetDeadlineUpdated(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long targetDeadline
	) = 0;
	virtual void printConnQuality(
		size_t ncoins, std::wstring const& connQualInfo
	) = 0;

	virtual void debugRoundTime(
		double theads_time
	) = 0;
	virtual void printBlockEnqueued(
		unsigned long long currentHeight,
		std::wstring const& coinname,
		bool atEnd, bool noQueue
	) = 0;
	virtual void printRoundInterrupt(
		unsigned long long currentHeight,
		std::wstring const& coinname
	) = 0;
	virtual void printRoundChangeInfo(bool isResumingInterrupted,
		unsigned long long currentHeight,
		std::wstring const& coinname,
		unsigned long long currentBaseTarget,
		unsigned long long currentNetDiff,
		bool isPoc2Round
	) = 0;

	void testmodeWarning(wchar_t const* const message)
	{
		printToConsole(2, true, false, true, false, message);
	}
	void testmodeError(wchar_t const* const message)
	{
		printToConsole(12, true, false, true, false, message);
	}
	void testmodeSuccess(wchar_t const* const message)
	{
		printToConsole(10, true, false, true, false, message);
	}
	void testmodeFinished()
	{
		printToConsole(2, false, true, true, false, L"TestMode has finished all tasks, press any key.");

		// after the last test, last status line is pending in console output writer
		// and the closeMiner will bm_end() which will interrupt the console writer
		// causing the last line to never show up.
		// Sadly, currently there's no better way to flush it other than wait
		system("pause > nul");
	}

	virtual bool currentlyDisplayingCorruptedPlotFiles() = 0;

	virtual int bm_wgetchMain() = 0; //get input vom main window

	virtual void showNewVersion(std::string version) = 0;

	virtual void printFileStats(std::map<std::wstring, t_file_stats>const& fileStats) = 0;

	// ---

	static std::wstring make_filled_string(int nspaces, wchar_t filler)
	{
		return std::wstring(std::max(0, nspaces), filler);
	}

	static std::wstring make_leftpad_for_networkstats(int availablespace, size_t nactivecoins)
	{
		int const todisplay = narrow_cast<int, size_t>(nactivecoins);
		int const remainingspace = availablespace - (todisplay * 4) - (todisplay - 1);
		return make_filled_string(remainingspace, L' ');
	}
};
