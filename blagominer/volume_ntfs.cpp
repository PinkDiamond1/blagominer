#include "stdafx.h"
#include "volume_ntfs.h"

#include <ntddvol.h>

// ShowError() and determineNtfsFilePosition()
// are based on https://cboard.cprogramming.com/windows-programming/95015-where-my-file-disk.html

wchar_t* ShowError(DWORD dw)
{
	static wchar_t szReturn[128] = { 0 };
	LPVOID lpMsgBuf;
	FormatMessage( // TODO: similar code in elevate.cpp
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	memset(szReturn, 0, sizeof(szReturn));
	wcscpy_s(szReturn, (LPTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return szReturn;
}

int determineNtfsFilePosition(LONGLONG& result, std::wstring drive, std::wstring filename)
{
	std::wstring szDrive = (drive + L"\\");
	std::wstring szVolumeHandleName = (L"\\\\.\\" + drive);
	std::wstring szFileName = filename;
	WCHAR szVolumeName[128] = { 0 };
	WCHAR szFileSystemName[32] = { 0 };
	INT iExtentsBufferSize = 1024;
	DWORD dwBytesReturned;
	DWORD dwSectorsPerCluster;
	DWORD dwBytesPerSector;
	DWORD dwNumberFreeClusters;
	DWORD dwTotalNumberClusters;
	DWORD dwVolumeSerialNumber;
	DWORD dwMaxFileNameLength;
	DWORD dwFileSystemFlags;
	DWORD dwClusterSizeInBytes;

	STARTING_VCN_INPUT_BUFFER StartingPointInputBuffer;
	VOLUME_LOGICAL_OFFSET VolumeLogicalOffset;
	LARGE_INTEGER  PhysicalOffsetReturnValue;
	VOLUME_PHYSICAL_OFFSETS VolumePhysicalOffsets;

	HANDLE hFile;

	hFile = CreateFile(
		szFileName.c_str(),
		GENERIC_READ /*| GENERIC_WRITE*/,
		FILE_SHARE_READ /*| FILE_SHARE_WRITE*/,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		Log(L"error: Cannot open file: %s, for %s", ShowError(GetLastError()), szFileName.c_str());
		return -1;
	}

	// Buffer to hold the extents info
	PRETRIEVAL_POINTERS_BUFFER lpRetrievalPointersBuffer =
		(PRETRIEVAL_POINTERS_BUFFER)malloc(iExtentsBufferSize);

	//StartingVcn field is the virtual cluster number in file
	// It is not a file offset

	// FSCTL_GET_RETRIEVAL_POINTERS acquires a list of virtual clusters from the
	// file system driver that make up the file and the related set of
	// Logical Clusters that represent the volume storage for these Virtual
	// Clusters.  On a heavliy fragmented volume, the file may not necessarily
	// be stored in contiguous storage locations.  Thus, it would be advisable
	// to follow the mapping of virtual to logical clusters "chain" to find
	// the complete physical layout of the file.

	// We want to start at the first virtual cluster ZERO   
	StartingPointInputBuffer.StartingVcn.QuadPart = 0;
	if (!DeviceIoControl(
		hFile,
		FSCTL_GET_RETRIEVAL_POINTERS,
		&StartingPointInputBuffer,
		sizeof(STARTING_VCN_INPUT_BUFFER),
		lpRetrievalPointersBuffer,
		iExtentsBufferSize,
		&dwBytesReturned,
		NULL))
	{
		Log(L"error: Cannot check file pointers: %s, for %s", ShowError(GetLastError()), szFileName.c_str());
		return -1;
	}
	CloseHandle(hFile);

	if (!GetDiskFreeSpace(
		szDrive.c_str(),
		&dwSectorsPerCluster,
		&dwBytesPerSector,
		&dwNumberFreeClusters,
		&dwTotalNumberClusters))
	{
		Log(L"error: Cannot check drive geometry: %s, for %s", ShowError(GetLastError()), szDrive.c_str());
		return -1;
	}
	dwClusterSizeInBytes = dwSectorsPerCluster * dwBytesPerSector;

	if (!GetVolumeInformation(
		szDrive.c_str(),
		szVolumeName,
		128,
		&dwVolumeSerialNumber,
		&dwMaxFileNameLength,
		&dwFileSystemFlags,
		szFileSystemName,
		32))
	{
		Log(L"error: Cannot check volume information: %s, for %s on %s", ShowError(GetLastError()), szVolumeName, szDrive.c_str());
		return -1;
	}

	HANDLE volumeHandle = CreateFile(
		szVolumeHandleName.c_str(),
		GENERIC_EXECUTE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (volumeHandle == INVALID_HANDLE_VALUE)
	{
		Log(L"error: Cannot open volume: %s, for %s", ShowError(GetLastError()), szVolumeHandleName.c_str());
		return -1;
	}

	else if (wcscmp(szFileSystemName, L"NTFS") == 0)
	{
		VolumeLogicalOffset.LogicalOffset = lpRetrievalPointersBuffer->Extents[0].Lcn.QuadPart * dwClusterSizeInBytes;
		if (!DeviceIoControl(
			volumeHandle,
			IOCTL_VOLUME_LOGICAL_TO_PHYSICAL,
			&VolumeLogicalOffset,
			sizeof(VOLUME_LOGICAL_OFFSET),
			&VolumePhysicalOffsets,
			sizeof(VOLUME_PHYSICAL_OFFSETS),
			&dwBytesReturned,
			NULL))
		{
			Log(L"error: Logical position analysis failed: %s, for: %s", ShowError(GetLastError()), szFileName.c_str());
			CloseHandle(volumeHandle);
			return -1;
		}
		CloseHandle(volumeHandle);
		PhysicalOffsetReturnValue.QuadPart = 0;
		PhysicalOffsetReturnValue.QuadPart += VolumePhysicalOffsets.PhysicalOffset[0].Offset;
		if (PhysicalOffsetReturnValue.QuadPart == -1)
		{
			Log(L"error: Physical file position analysis failed: %s, for: %s", ShowError(GetLastError()), szFileName.c_str());
			return  -1;
		}
		else
		{
			//Log(L"%s starts at 0x%08x%08x from beginning of the disk",
			//	szFileName.c_str(),
			//	PhysicalOffsetReturnValue.HighPart,
			//	PhysicalOffsetReturnValue.LowPart);
			result = PhysicalOffsetReturnValue.QuadPart;
			return 0;
		}
	}
	else
	{
		Log(L"error: File system NOT supported: %s", szFileSystemName);
		return  -1;
	}
}
