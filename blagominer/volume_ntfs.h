#pragma once
#include "common-pragmas.h"

#include "logger.h"
#include "inout.h"

struct OrderingByPositionOnSequentialMedia
{
	inline bool operator() (const t_files& struct1, const t_files& struct2)
	{
		return (struct1.positionOnNtfsVolume < struct2.positionOnNtfsVolume);
	}
};

int determineNtfsFilePosition(LONGLONG& result, std::wstring drive, std::wstring filename);
