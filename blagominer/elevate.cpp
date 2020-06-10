#include "stdafx.h"
#include "elevate.h"

#include "logger.h"
#include "inout.h"

bool RestartWithElevation(int argc, wchar_t **argv) {
	if (argc < 0 || !argv || !argv[0]) {
		// TODO: log: panic
		return false;
	}

	std::wstring executable;
	ArgvQuote(argv[0], executable, false);

	std::wstring params;
	for (int i = 1; i < argc; ++i) { ArgvQuote(argv[i], params, false); params += L" "; }

	// https://docs.microsoft.com/en-us/windows/desktop/api/shellapi/nf-shellapi-shellexecutea#return-value
	int result = (int)ShellExecute(NULL,
		L"runas",
		executable.c_str(),
		params.c_str(),
		NULL,
		SW_SHOWNORMAL
	);

	if (result > 32) {
		// TODO: log elevation+restart success
		return true;
	}

	// https://docs.microsoft.com/pl-pl/windows/desktop/Debug/retrieving-the-last-error-code
	LPVOID lpMsgBuf;
	DWORD dw = result;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	std::wstring errinfo = L"Self-elevation attempt failed: ";
	errinfo += (wchar_t*)lpMsgBuf;

	Log(errinfo.c_str());

	LocalFree(lpMsgBuf);
	return false;
}

// https://stackoverflow.com/a/8196291/717732
bool IsElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet == TRUE;
}

// https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
void
ArgvQuote(
	const std::wstring& Argument,
	std::wstring& CommandLine,
	bool Force
)
/*++

Routine Description:

	This routine appends the given argument to a command line such
	that CommandLineToArgvW will return the argument string unchanged.
	Arguments in a command line should be separated by spaces; this
	function does not add these spaces.

Arguments:

	Argument - Supplies the argument to encode.

	CommandLine - Supplies the command line to which we append the encoded argument string.

	Force - Supplies an indication of whether we should quote
			the argument even if it does not contain any characters that would
			ordinarily require quoting.

Return Value:

	None.

Environment:

	Arbitrary.

--*/
{
	//
	// Unless we're told otherwise, don't quote unless we actually
	// need to do so --- hopefully avoid problems if programs won't
	// parse quotes properly
	//

	if (Force == false &&
		Argument.empty() == false &&
		Argument.find_first_of(L" \t\n\v\"") == Argument.npos)
	{
		CommandLine.append(Argument);
	}
	else {
		CommandLine.push_back(L'"');

		for (auto It = Argument.begin(); ; ++It) {
			unsigned NumberBackslashes = 0;

			while (It != Argument.end() && *It == L'\\') {
				++It;
				++NumberBackslashes;
			}

			if (It == Argument.end()) {

				//
				// Escape all backslashes, but let the terminating
				// double quotation mark we add below be interpreted
				// as a metacharacter.
				//

				CommandLine.append(NumberBackslashes * 2, L'\\');
				break;
			}
			else if (*It == L'"') {

				//
				// Escape all backslashes and the following
				// double quotation mark.
				//

				CommandLine.append(NumberBackslashes * 2 + 1, L'\\');
				CommandLine.push_back(*It);
			}
			else {

				//
				// Backslashes aren't special here.
				//

				CommandLine.append(NumberBackslashes, L'\\');
				CommandLine.push_back(*It);
			}
		}

		CommandLine.push_back(L'"');
	}
}
