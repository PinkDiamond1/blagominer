#include "error.h"

#include "logger.h"
#include "inout.h"

void ShowMemErrorExit(void)
{
	Log(L"!!! Error allocating memory");
	gui->printToConsole(12, false, true, true, false, L"Error allocating memory");
	system("pause > nul");
	exit(-1);
}
