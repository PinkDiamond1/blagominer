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

#include "heapallocator.h"
#include "logger.h"

#include <curl/curl.h>

// blago version
extern const unsigned int versionMajor;
extern const unsigned int versionMinor;
extern const unsigned int versionRevision;
extern std::wstring version;

extern const wchar_t sepChar;

extern double checkForUpdateInterval;
extern bool ignoreSuspectedFastBlocks;
extern volatile bool exit_flag;							// true if miner is to be exited

extern HANDLE hHeap;							//heap
extern heap_allocator<char> theHeap;

enum MiningState {
	QUEUED,
	MINING,
	DONE,
	INTERRUPTED
};

struct t_shares {
	t_shares(std::string file_name, unsigned long long account_id, unsigned long long best,
		unsigned long long nonce, unsigned long long deadline, unsigned long long height,
		unsigned long long baseTarget) :
		file_name(file_name), account_id(account_id), best(best), nonce(nonce), deadline(deadline),
		height(height), baseTarget(baseTarget) {}
	std::string file_name;
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
	std::string Path;
	std::string Name;
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
	t_directory_info(std::string dir, bool done, std::vector<t_files> files) : dir(dir), done(done), files(files) {}
	std::string dir;
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
	std::string filename;
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

	CRITICAL_SECTION sessionsLock;			// session lock
	CRITICAL_SECTION sessions2Lock;			// session2 lock
	CRITICAL_SECTION bestsLock;				// best lock
	CRITICAL_SECTION sharesLock;			// shares lock

	volatile bool stopRoundSpecificNetworkingThreads;
};

struct t_mining_info {
	bool enable;
	bool newMiningInfoReceived;
	size_t miner_mode;						// miner mode. 0=solo, 1=pool
	size_t priority;
	MiningState state;
	unsigned long long baseTarget;			// base target of current block
	unsigned long long targetDeadlineInfo;	// target deadline info from pool
	unsigned long long height;				// current block height
	unsigned long long deadline;			// current deadline
	unsigned long long my_target_deadline;
	unsigned long long POC2StartBlock;
	bool enableDiskcoinGensigs;
	unsigned int scoop;						// currenty scoop
	std::vector<std::shared_ptr<t_directory_info>> dirs;

	// Values for current mining process
	char currentSignature[33];
	char current_str_signature[65];
	unsigned long long currentHeight = 0;
	unsigned long long currentBaseTarget = 0;
	std::vector<t_best> bests;
	std::vector<std::shared_ptr<t_shares>> shares;

	// Values for new mining info check
	char signature[33];						// signature of current block
	char oldSignature[33];					// signature of last block
	char str_signature[65];
};

struct t_network_info {
	std::string nodeaddr;
	std::string nodeport;
	std::string noderoot;
	std::string solopass;
	unsigned submitTimeout;
	std::string updateraddr;
	std::string updaterport;
	std::string updaterroot;
	bool enable_proxy;						// enable client/server functionality
	std::string proxyport;
	size_t send_interval;
	size_t update_interval;
	size_t proxy_update_interval;
	int network_quality;
	bool usehttps;
	std::string sendextraquery;
	std::string sendextraheader;
	std::vector<std::shared_ptr<t_session>> sessions;
	std::vector<std::shared_ptr<t_session2>> sessions2;
	std::thread sender;
	std::thread confirmer;
};

struct t_roundreplay_round;
struct t_roundreplay_round_test;

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

	t_roundreplay_round* testround1;
	t_roundreplay_round_test* testround2;
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
	enum RoundTestMode { RMT_UNKNOWN = 0, RMT_NORMAL = 1, RMT_OFFLINE = 2 };
	RoundTestMode mode;

	unsigned long long assume_account;
	unsigned long long assume_nonce;

	std::optional<unsigned int> assume_scoop;
	std::optional<std::string> assume_scoop_low;
	std::optional<std::string> assume_scoop_high;

	std::optional<unsigned int> check_scoop;
	std::optional<std::string> check_scoop_low;
	std::optional<std::string> check_scoop_high;
	std::optional<unsigned long long> check_deadline;

	std::optional<bool> passed_scoop;
	std::optional<bool> passed_scoop_low;
	std::optional<bool> passed_scoop_high;
	std::optional<bool> passed_deadline;
};

struct t_roundreplay_round {
	unsigned long long height;
	std::string signature;
	unsigned long long baseTarget;

	std::optional<bool> assume_POC2; // TODO: since it's round X test, move that to the TEST // obvious in OFFLINE, and in ONLINE remember the mode is SHORT not FULL!

	std::vector<t_roundreplay_round_test> tests;
};

struct t_roundreplay {
	bool isEnabled;
	std::wstring coinName;
	std::vector<t_roundreplay_round> rounds;
};

struct t_testmode_config {
	bool isEnabled;
	t_roundreplay roundReplay;
};

extern t_testmode_config testmodeConfig;


unsigned long long getHeight(std::shared_ptr<t_coin_info> coin);
void setHeight(std::shared_ptr<t_coin_info> coin, const unsigned long long height);

unsigned long long getTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin);
void setTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin, const unsigned long long targetDeadlineInfo);
char* getSignature(std::shared_ptr<t_coin_info> coin);
char* getCurrentStrSignature(std::shared_ptr<t_coin_info> coin);
void setSignature(std::shared_ptr<t_coin_info> coin, const char* signature);
void setStrSignature(std::shared_ptr<t_coin_info> coin, const char* signature);
void updateOldSignature(std::shared_ptr<t_coin_info> coin);
void updateCurrentStrSignature(std::shared_ptr<t_coin_info> coin);
bool signaturesDiffer(std::shared_ptr<t_coin_info> coin);
bool signaturesDiffer(std::shared_ptr<t_coin_info> coin, const char* sig);
bool haveReceivedNewMiningInfo(const std::vector<std::shared_ptr<t_coin_info>>& coins);
void setnewMiningInfoReceived(std::shared_ptr<t_coin_info> coin, const bool val);
int getNetworkQuality(std::shared_ptr<t_coin_info> coin);

void getLocalDateTime(const std::time_t &rawtime, char* local, const std::string sepTime = ":");

std::wstring toWStr(int number, const unsigned short length);
std::wstring toWStr(unsigned long long number, const unsigned short length);
std::wstring toWStr(std::wstring str, const unsigned short length);
std::wstring toWStr(std::string str, const unsigned short length);

std::string toStr(unsigned long long number, const unsigned short length);
std::string toStr(std::string str, const unsigned short length);

struct IUserInterface;

extern std::unique_ptr<IUserInterface> gui;

struct IUserInterface
{
	virtual ~IUserInterface() = 0;

	virtual void printToConsole(int colorPair, bool printTimestamp, bool leadingNewLine,
		bool trailingNewLine, bool fillLine, const wchar_t * format, ...) = 0;
	virtual void printThreadActivity(
		std::wstring const& coinName,
		std::wstring const& threadKind,
		std::wstring const& threadAction
	) = 0;
	virtual void debugWorkerStats(
		std::wstring const& specialReadMode,
		std::string const& directory,
		double proc_time, double work_time,
		unsigned long long files_size_per_thread
	) = 0;
	virtual void printWorkerDeadlineFound(
		unsigned long long account_id,
		std::wstring const& coinname,
		unsigned long long deadline
	) = 0;
	virtual void printNetworkProxyDeadlineReceived(
		unsigned long long account_id,
		std::wstring const& coinName,
		unsigned long long deadline,
		char const (& const clientAddr)[22]
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
	virtual void debugRoundTime(
		double theads_time
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

	virtual void printConnQuality(int ncoins, std::wstring const& connQualInfo) = 0;
	virtual void printScanProgress(int ncoins, std::wstring const& connQualInfo,
		unsigned long long bytesRead, unsigned long long round_size,
		double thread_time, double threads_speed,
		unsigned long long deadline) = 0;

	virtual bool currentlyDisplayingCorruptedPlotFiles() = 0;

	virtual int bm_wgetchMain() = 0; //get input vom main window

	virtual void showNewVersion(std::string version) = 0;

	virtual void printFileStats(std::map<std::string, t_file_stats>const& fileStats) = 0;

	// ---

	static std::wstring make_filled_string(int nspaces, wchar_t filler)
	{
		return std::wstring(max(0, nspaces), filler);
	}

	static std::wstring make_leftpad_for_networkstats(int availablespace, int nactivecoins)
	{
		const int remainingspace = availablespace - (nactivecoins * 4) - (nactivecoins - 1);
		return make_filled_string(remainingspace, L' ');
	}
};
