#pragma once
#include "common-pragmas.h"

#include "logger.h"
#include "inout.h"

bool RestartWithElevation(int argc, wchar_t **argv);

BOOL IsElevated();

void
ArgvQuote(
	const std::wstring& Argument,
	std::wstring& CommandLine,
	bool Force
);