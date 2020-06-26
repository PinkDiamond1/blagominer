#include "stdafx.h"
#include "common.h"

#include "line_noise_convert_utf.h"

const wchar_t sepChar = 0x00B7;

double checkForUpdateInterval = 1;
bool ignoreSuspectedFastBlocks = true;
volatile bool exit_flag = false;

HANDLE hHeap;
heap_allocator<char> theHeap;

static unsigned long long getHeight(t_coin_info& coin) {
	std::lock_guard<std::mutex> lockGuard(coin.locks->mHeight);
	return coin.mining->height;
}
bool t_coin_info::isPoc2Round()
{
	//POC2 determination
	if (getHeight(*this) >= this->mining->POC2StartBlock) {
		return true;
	}
	return false;
}

unsigned long long getHeight(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mHeight);
	return coin->mining->height;
}
void setHeight(std::shared_ptr<t_coin_info> coin, const unsigned long long height) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mHeight);
	coin->mining->height = height;
}

unsigned long long getTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mTargetDeadlineInfo);
	return coin->mining->targetDeadlineInfo;
}
void setTargetDeadlineInfo(std::shared_ptr<t_coin_info> coin, const unsigned long long targetDeadlineInfo) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mTargetDeadlineInfo);
	coin->mining->targetDeadlineInfo = targetDeadlineInfo;
}

/**
	Don't forget to delete the pointer after using it. // TODO: christ, really?
**/
char* getSignature(std::shared_ptr<t_coin_info> coin) {
	char* sig = new char[33];
	RtlSecureZeroMemory(sig, 33);
	{
		std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
		memmove(sig, coin->mining->signature, 32);
	}
	return sig;
}

std::wstring getCurrentStrSignature(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mCurrentStrSignature);
	return coin->mining->current_str_signature;
}

void setSignature(std::shared_ptr<t_coin_info> coin, const char* signature) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	memmove(coin->mining->signature, signature, 32);
}
void setStrSignature(std::shared_ptr<t_coin_info> coin, std::wstring const& str_signature) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mStrSignature);
	coin->mining->str_signature = str_signature;
}

void updateOldSignature(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	std::lock_guard<std::mutex> lockGuardO(coin->locks->mOldSignature);
	memmove(coin->mining->oldSignature, coin->mining->signature, 32);
}

void updateCurrentStrSignature(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mStrSignature);
	std::lock_guard<std::mutex> lockGuardO(coin->locks->mCurrentStrSignature);
	coin->mining->current_str_signature = coin->mining->str_signature;
}

bool signaturesDiffer(std::shared_ptr<t_coin_info> coin) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	std::lock_guard<std::mutex> lockGuardO(coin->locks->mOldSignature);
	return memcmp(coin->mining->signature, coin->mining->oldSignature, 32) != 0;
}

bool signaturesDiffer(std::shared_ptr<t_coin_info> coin, const char* sig) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mSignature);
	return memcmp(coin->mining->signature, sig, 32) != 0;
}

bool haveReceivedNewMiningInfo(const std::vector<std::shared_ptr<t_coin_info>>& coins) {
	for (auto& e : coins) {
		std::lock_guard<std::mutex> lockGuard(e->locks->mNewMiningInfoReceived);
		if (e->mining->newMiningInfoReceived) {
			return true;
		}
	}
	return false;
}
void setnewMiningInfoReceived(std::shared_ptr<t_coin_info> coin, const bool val) {
	std::lock_guard<std::mutex> lockGuard(coin->locks->mNewMiningInfoReceived);
	coin->mining->newMiningInfoReceived = val;
}

int getNetworkQuality(std::shared_ptr<t_coin_info> coin) {
	if (coin->network->network_quality < 0) {
		return 0;
	}
	return coin->network->network_quality;
}

void getLocalDateTime(const std::time_t &rawtime, char* local, const std::string sepTime) {
	struct tm timeinfo;

	localtime_s(&timeinfo, &rawtime);
	// YYYY-mm-dd HH:MM:SS
	std::string format = "%Y-%m-%d %H" + sepTime + "%M" + sepTime + "%S";
	strftime(local, 80, format.c_str(), &timeinfo);
}

std::wstring toWStr(int number, const unsigned short length) {
	std::wstring s = std::to_wstring(number);
	std::wstring prefix;

	if (length == 0) {
		return L"";
	}
	else if (length == 1 && s.size() > 1) {
		return L">";
	} else if (s.size() > length) {
		s = std::to_wstring(static_cast<int>(pow(10, length - 1)) - 1);
		prefix = L">";
	}
	else if (s.size() < length) {
		prefix = std::wstring(length - s.size(), L' ');
	}
	
	return prefix + s;
}

std::wstring toWStr(unsigned long long number, const unsigned short length) {
	return toWStr(toStr(number, length));
}

std::wstring toWStr(std::wstring str, const unsigned short length) {
	if (str.size() > length) {
		if (length > 3) {
			str = L"..." + std::wstring(str.end() - length + 3, str.end());
		}
		else {
			str = std::wstring(length, L'.');
		}
	}
	else if (str.size() < length) {
		str = str + std::wstring(length - str.size(), L' ');
	}
	return str;
}

// TODO: test me: basics + endianess [because windows expect wchar_t to be UTF16LE]
// TODO: test me: something that will fool initial size prediction and force targetExhausted branch
// TODO: test me: invalid source characters
// TODO: test me: source exhausted branch
std::wstring toWStr(std::string narrow_utf8_source_string) {
	using namespace linenoise_ng;

	std::vector<uint8_t> source(narrow_utf8_source_string.begin(), narrow_utf8_source_string.end());
	uint8_t const* srcfrom = source.data();
	uint8_t const* srcend = srcfrom + source.size();

	std::vector<uint16_t> buf(narrow_utf8_source_string.size());
	uint16_t* tgtfrom = buf.data();
	uint16_t* tgtend = buf.data() + buf.size();

retry:
	switch (convert_utf8_to_utf16(&srcfrom, srcend, &tgtfrom, tgtend, conversion_flags::strictConversion))
	{
	case conversion_result::targetExhausted:
	{
		auto lastoffset = tgtfrom - buf.data();
		buf.resize(buf.size() + (srcend - srcfrom)); // invalidates pointers: tgtfrom,tgtend
		tgtfrom = buf.data() + lastoffset;
		tgtend = buf.data() + buf.size();
		goto retry;
	}
	
	case conversion_result::sourceIllegal:
	{
		// when the source data seems invalid, let's just widen each character in a dumb way
		// at least any simple characters will be preserved
		std::wstring tmp = L"!cvt!:";
		tmp += std::wstring(narrow_utf8_source_string.begin(), narrow_utf8_source_string.end());
		return tmp;
	}

	case conversion_result::sourceExhausted:
	case conversion_result::conversionOK:
		buf.resize(tgtfrom - buf.data());
		break;
	}

	return std::wstring(buf.begin(), buf.end());
}

std::string toStr(unsigned long long number, const unsigned short length) {
	std::string s = std::to_string(number);
	std::string prefix;

	if (length == 0) {
		return "";
	}
	else if (length == 1 && s.size() > 1) {
		return ">";
	}
	else if (s.size() > length) {
		s = std::to_string(static_cast<unsigned long long>(pow(10, length - 1)) - 1);
		prefix = ">";
	}
	else if (s.size() < length) {
		prefix = std::string(length - s.size(), ' ');
	}

	return prefix + s;
}

std::string toStr(std::string str, const unsigned short length) {
	if (str.size() > length) {
		if (length > 3) {
			str = "..." + std::string(str.end() - length + 3, str.end());
		}
		else {
			str = std::string(length, '.');
		}
	}
	else if (str.size() < length) {
		str = str + std::string(length - str.size(), ' ');
	}
	return str;
}

// TODO: test me: basics + endianess [because windows expect wchar_t to be UTF16LE]
// TODO: test me: something that will fool initial size prediction and force targetExhausted branch
// TODO: test me: invalid source characters
// TODO: test me: source exhausted branch
std::string toStr(std::wstring wide_utf16_source_string) {
	using namespace linenoise_ng;

	std::vector<uint16_t> source(wide_utf16_source_string.begin(), wide_utf16_source_string.end());
	uint16_t const* srcfrom = source.data();
	uint16_t const* srcend = srcfrom + source.size();

	std::vector<uint8_t> buf(wide_utf16_source_string.size());
	uint8_t* tgtfrom = buf.data();
	uint8_t* tgtend = buf.data() + buf.size();

retry:
	switch (convert_utf16_to_utf8(&srcfrom, srcend, &tgtfrom, tgtend, conversion_flags::strictConversion))
	{
	case conversion_result::targetExhausted:
	{
		auto lastoffset = tgtfrom - buf.data();
		buf.resize(buf.size() + (srcend - srcfrom)); // invalidates pointers: tgtfrom,tgtend
		tgtfrom = buf.data() + lastoffset;
		tgtend = buf.data() + buf.size();
		goto retry;
	}

	case conversion_result::sourceIllegal:
	{
		// when the source data seems invalid, let's just strip each character in a dumb way
		// at least any simple characters will be preserved
		std::string tmp = "!cvt!:";
		tmp += std::string(wide_utf16_source_string.begin(), wide_utf16_source_string.end());
		return tmp;
	}

	case conversion_result::sourceExhausted:
	case conversion_result::conversionOK:
		buf.resize(tgtfrom - buf.data());
		break;
	}

	return std::string(buf.begin(), buf.end());
}
