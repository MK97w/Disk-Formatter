#pragma once

#include <windows.h>
#include <winioctl.h>   // for MEDIA_TYPE
#include "fat32_format.h"
#include "drive.h"

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
    void formatDrive(const Drive& ,const std::wstring&);
    BOOL FMIFS_Format(const wchar_t* driveRoot, const wchar_t* fileSystem, const wchar_t* volumeLabel, DWORD clusterSize);
    BOOL Large_FAT32_Format(  std::wstring & driveRoot, const std::wstring label);
private:
    DWORD getClusterSize(uint64_t, const std::wstring& targetFS);
private:
    constexpr static uint64_t KB = 1024;
    constexpr static uint64_t MB = 1024 * KB;
    constexpr static uint64_t GB = 1024 * MB;
    constexpr static uint64_t TB = 1024 * GB;
};  