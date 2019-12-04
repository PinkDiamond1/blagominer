#pragma once
#include "common-pragmas.h"

#include <string>

bool RestartWithElevation(int argc, wchar_t **argv);

bool IsElevated();

void
ArgvQuote(
	const std::wstring& Argument,
	std::wstring& CommandLine,
	bool Force
);
