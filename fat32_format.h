// Fat32 formatter version 1.05
// (c) Tom Thornhill 2007,2008,2009
// This software is covered by the GPL. 
// By using this tool, you agree to absolve Ridgecrop of an liabilities for lost data.
// Please backup any data you value before using this tool.

// |                      ALIGNING_SIZE * N                      |
// | BPB,FSInfo,Reserved | FAT1              | FAT2              | Cluster0
static const int ALIGNING_SIZE = 1024 * 1024;

#define WIN32_LEAN_AND_MEAN
#define STRICT_GS_ENABLED
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES        1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT  1
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES          1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_MEMORY 1
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES_MEMORY   1


#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winioctl.h>
#include <versionhelpers.h>

struct FAT_BOOTSECTOR32
{
	// Common fields.
	BYTE sJmpBoot[3];
	CHAR sOEMName[8];
	WORD wBytsPerSec;
	BYTE bSecPerClus;
	WORD wRsvdSecCnt;
	BYTE bNumFATs;
	WORD wRootEntCnt;
	WORD wTotSec16; // if zero, use dTotSec32 instead
	BYTE bMedia;
	WORD wFATSz16;
	WORD wSecPerTrk;
	WORD wNumHeads;
	DWORD dHiddSec;
	DWORD dTotSec32;
	// Fat 32/16 only
	DWORD dFATSz32;
	WORD wExtFlags;
	WORD wFSVer;
	DWORD dRootClus;
	WORD wFSInfo;
	WORD wBkBootSec;
	BYTE Reserved[12];
	BYTE bDrvNum;
	BYTE Reserved1;
	BYTE bBootSig; // == 0x29 if next three fields are ok
	DWORD dBS_VolID;
	BYTE sVolLab[11];
	BYTE sBS_FilSysType[8];
};

struct FAT_FSINFO
{
	DWORD dLeadSig;         // 0x41615252
	BYTE sReserved1[480];   // zeros
	DWORD dStrucSig;        // 0x61417272
	DWORD dFree_Count;      // 0xFFFFFFFF
	DWORD dNxt_Free;        // 0xFFFFFFFF
	BYTE sReserved2[12];    // zeros
	DWORD dTrailSig;        // 0xAA550000
};

struct FAT_DIRECTORY
{
	UINT8  DIR_Name[8 + 3];
	UINT8  DIR_Attr;
	UINT8  DIR_NTRes;
	UINT8  DIR_CrtTimeTenth;
	UINT16 DIR_CrtTime;
	UINT16 DIR_CrtDate;
	UINT16 DIR_LstAccDate;
	UINT16 DIR_FstClusHI;
	UINT16 DIR_WrtTime;
	UINT16 DIR_WrtDate;
	UINT16 DIR_FstClusLO;
	UINT32 DIR_FileSize;
};
static_assert(sizeof(FAT_DIRECTORY) == 32);

#pragma pack(pop)


struct format_params
{
	int sectors_per_cluster = 0;        // can be zero for default or 1,2,4,8,16,32 or 64
	bool make_protected_autorun = false;
	bool all_yes = false;
	char volume_label[sizeof(FAT_BOOTSECTOR32::sVolLab) + 1] = {};
};

/*
28.2  CALCULATING THE VOLUME SERIAL NUMBER

For example, say a disk was formatted on 26 Dec 95 at 9:55 PM and 41.94
seconds.  DOS takes the date and time just before it writes it to the
disk.

Low order word is calculated:               Volume Serial Number is:
	Month & Day         12/26   0c1ah
	Sec & Hundrenths    41:94   295eh               3578:1d02
								-----
								3578h

High order word is calculated:
	Hours & Minutes     21:55   1537h
	Year                1995    07cbh
								-----
								1d02h
*/


	DWORD get_volume_id();
	DWORD get_fat_size_sectors(_In_ DWORD DskSize, _In_ DWORD ReservedSecCnt, _In_ DWORD SecPerClus, _In_ DWORD NumFATs, _In_ DWORD BytesPerSect);
	void seek_to_sect(_In_ HANDLE hDevice, _In_ DWORD Sector, _In_ DWORD BytesPerSect);
	void write_sect(_In_ HANDLE hDevice, _In_ DWORD Sector, _In_ DWORD BytesPerSector, _In_ void* Data, _In_ DWORD NumSects);
	void zero_sectors(_In_ HANDLE hDevice, _In_ DWORD Sector, _In_  DWORD BytesPerSect, _In_ DWORD NumSects);
	BYTE get_spc(_In_ DWORD ClusterSizeKB, _In_ DWORD BytesPerSect);
	BYTE get_sectors_per_cluster(_In_ LONGLONG DiskSizeBytes, _In_ DWORD BytesPerSect);
	int format_volume(_In_z_ LPCSTR vol, _In_ const format_params* params);
	[[noreturn]] void die(_In_z_ PCSTR error);
