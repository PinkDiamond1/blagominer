#include "stdafx.h"
#include "updateChecker.h"

#include "common.h"
#include "blagominer_meta.h"


LPCWSTR versionUrl = L"https://raw.githubusercontent.com/quetzalcoatl/blagominer/master/.version";

double getDiffernceinDays(const std::time_t end, std::time_t beginning) {
	return std::difftime(end, beginning) / (60llu * 60 * 24);
}

// Source partially from http://www.rohitab.com/discuss/topic/28719-downloading-a-file-winsock-http-c/page-2#entry10072081
void checkForUpdate() {

	std::time_t lastChecked = 0;
	while (!exit_flag)
	{
		if (getDiffernceinDays(std::time(nullptr), lastChecked) > checkForUpdateInterval)
		{
			std::this_thread::yield();
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		lastChecked = std::time(nullptr);

		Log(L"UPDATE CHECKER: Checking for new version.");
		std::unique_ptr<IStream, void(*)(LPSTREAM)> guardStream(nullptr, [](LPSTREAM lpStream) { lpStream->Release(); });
		{
			LPSTREAM tmp;
			if (FAILED(URLOpenBlockingStream(NULL, versionUrl, &tmp, 0, NULL)))
			{
				Log(L"UPDATE CHECKER: Error opening stream.");
				continue;
			}
			guardStream.reset(tmp);
		}

		STATSTG statStream;
		if (FAILED(guardStream->Stat(&statStream, STATFLAG_NONAME)))
		{
			Log(L"UPDATE CHECKER: Error retrieving stream data.");
			continue;
		}

		DWORD dwSize = statStream.cbSize.LowPart;
		std::vector<char> lpResult(dwSize);

		guardStream->Seek({ 0 }, STREAM_SEEK_SET, NULL);
		guardStream->Read(lpResult.data(), dwSize, NULL);

		DocumentUTF16LE document = parseJsonData<kParseNoFlags>(lpResult);
		if (document.HasParseError())
		{
			Log(L"UPDATE CHECKER: Error (offset %zu) parsing retrieved data: %S", document.GetErrorOffset(), GetParseError_En(document.GetParseError()));
			continue;
		}

		if (!document.IsObject())
		{
			Log(L"UPDATE CHECKER: Error parsing release version data: root is not an object.");
			continue;
		}

		if (!document.HasMember(L"major") ||
			!document.HasMember(L"minor") ||
			!document.HasMember(L"revision"))
		{
			Log(L"UPDATE CHECKER: Error parsing release version number.");
			continue;
		}

		if (!document[L"major"].IsUint() ||
			!document[L"minor"].IsUint() ||
			!document[L"revision"].IsUint())
		{
			Log(L"UPDATE CHECKER: Error parsing release version number.");
			continue;
		}

		unsigned int major = document[L"major"].GetUint();
		unsigned int minor = document[L"minor"].GetUint();
		unsigned int revision = document[L"revision"].GetUint();

		if (major > versionMajor ||
			(major >= versionMajor && minor > versionMinor) ||
			(major >= versionMajor && minor >= versionMinor && revision > versionRevision))
		{
			std::wstring releaseVersion = std::to_wstring(major) + L"." + std::to_wstring(minor) + L"." + std::to_wstring(revision);
			Log(L"UPDATE CHECKER: New version availabe: %s", releaseVersion.c_str());
			gui->showNewVersion(toStr(releaseVersion));
		}
		else
		{
			Log(L"UPDATE CHECKER: The miner is up to date (%i.%i.%i)", versionMajor, versionMinor, versionRevision);
		}
	}

}
