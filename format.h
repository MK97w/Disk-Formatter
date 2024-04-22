#pragma once

#include <windows.h>
#include <winioctl.h>   // for MEDIA_TYPE
#include <string>
#include "fat32_format.h"

enum CALLBACKCOMMAND {
    PROGRESS,
    DONEWITHSTRUCTURE,
    UNKNOWN2,
    UNKNOWN3,
    UNKNOWN4,
    UNKNOWN5,
    INSUFFICIENTRIGHTS,
    UNKNOWN7,
    UNKNOWN8,
    UNKNOWN9,
    UNKNOWNA,
    DONE, // format OK!
    UNKNOWNC,
    UNKNOWND,
    OUTPUT,
    STRUCTUREPROGRESS
};

typedef BOOLEAN(WINAPI* PFMIFSCALLBACK)(CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo);

enum class FMIFS_MEDIA_TYPE
{
    Unknown = MEDIA_TYPE::Unknown,                   // Format is unknown
    RemovableMedia = MEDIA_TYPE::RemovableMedia,     // Removable media other than floppy
    FixedMedia = MEDIA_TYPE::FixedMedia,             // Fixed hard disk media
};

struct FORMAT_DATA
{
    BOOLEAN fOk;
};


class VolumeFormatter
{
public:
    static inline ULONG g_dwTlsIndex = TLS_OUT_OF_INDEXES;

public:
    VolumeFormatter();
    ~VolumeFormatter();
    BOOL FMIFS_Format(const wchar_t* driveRoot, const wchar_t* fileSystem, const wchar_t* volumeLabel, DWORD clusterSize);
    BOOL formatLarge_FAT32(LPCSTR driveRoot);
};