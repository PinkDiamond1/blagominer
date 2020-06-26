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
	while (!exit_flag) {
		bool error = false;
		if (getDiffernceinDays(std::time(nullptr), lastChecked) > checkForUpdateInterval) {
			Log(L"UPDATE CHECKER: Checking for new version.");
			LPSTR lpResult = NULL;
			DWORD dwSize = 0;
			LPSTREAM lpStream;
			if (SUCCEEDED(URLOpenBlockingStream(NULL, versionUrl, &lpStream, 0, NULL))) {
				STATSTG statStream;
				if (SUCCEEDED(lpStream->Stat(&statStream, STATFLAG_NONAME))) {
					dwSize = statStream.cbSize.LowPart;
					lpResult = (LPSTR)malloc(dwSize + 1); // TODO: LPRESULT seem to be never free'd
					if (lpResult) {
						LARGE_INTEGER liPos;
						ZeroMemory(&liPos, sizeof(liPos));
						ZeroMemory(lpResult, dwSize + 1);
						lpStream->Seek(liPos, STREAM_SEEK_SET, NULL);
						lpStream->Read(lpResult, dwSize, NULL);
					}
					else {
						Log(L"UPDATE CHECKER: Error allocating memory.");
						error = true;
					}
				}
				else {
					Log(L"UPDATE CHECKER: Error retrieving stream data.");
					error = true;
				}
				lpStream->Release();

			}
			else {
				Log(L"UPDATE CHECKER: Error opening stream.");
				error = true;
			}

			if (!error) {
				DocumentUTF16LE document = parseJsonData<kParseNoFlags>(span{ (char const*)lpResult, dwSize });
				if (document.HasParseError()) {
					Log(L"UPDATE CHECKER: Error (offset %zu) parsing retrieved data: %S", document.GetErrorOffset(), GetParseError_En(document.GetParseError()));
				}
				else {
					if (document.IsObject()) {
						if (document.HasMember(L"major") && document[L"major"].IsUint() &&
							document.HasMember(L"minor") && document[L"minor"].IsUint() &&
							document.HasMember(L"revision") && document[L"revision"].IsUint()) {
							
							unsigned int major = document[L"major"].GetUint();
							unsigned int minor = document[L"minor"].GetUint();
							unsigned int revision = document[L"revision"].GetUint();
							
							if (major > versionMajor ||
								(major >= versionMajor && minor > versionMinor) ||
								(major >= versionMajor && minor >= versionMinor && revision > versionRevision)) {
								std::wstring releaseVersion =
									std::to_wstring(major) + L"." + std::to_wstring(minor) + L"." + std::to_wstring(revision);
								Log(L"UPDATE CHECKER: New version availabe: %s", releaseVersion.c_str());
								gui->showNewVersion(toStr(releaseVersion));
							}
							else {
								Log(L"UPDATE CHECKER: The miner is up to date (%i.%i.%i)", versionMajor, versionMinor, versionRevision);
							}
						}
						else {
							Log(L"UPDATE CHECKER: Error parsing release version number.");
						}
					}
				}
			}
			lastChecked = std::time(nullptr);
		}
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

}
