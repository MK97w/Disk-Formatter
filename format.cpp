#include "format.h"


VolumeFormatter::VolumeFormatter() {}

VolumeFormatter::~VolumeFormatter()
{
    if (g_dwTlsIndex != TLS_OUT_OF_INDEXES)
    {
        TlsFree(g_dwTlsIndex);
    }
}

BOOLEAN FormatCallback(CALLBACKCOMMAND Command, DWORD SubAction, PVOID ActionInfo)
{
    FORMAT_DATA* fd = (FORMAT_DATA*)TlsGetValue(VolumeFormatter::g_dwTlsIndex);
    if (Command == DONE)
    {
        fd->fOk = TRUE;
    }
    return TRUE;
}


BOOL VolumeFormatter::FMIFS_Format(const wchar_t* driveRoot, const wchar_t* fileSystem, const wchar_t* volumeLabel, DWORD clusterSize)
{
    FORMAT_DATA fd;
    fd.fOk = FALSE;

    if ((g_dwTlsIndex = TlsAlloc()) != TLS_OUT_OF_INDEXES)
    {
        if (HMODULE hmod = LoadLibrary(L"fmifs"))
        {
            VOID(WINAPI * FormatEx)(
                PCWSTR DriveRoot,
                FMIFS_MEDIA_TYPE MediaType,
                PCWSTR FileSystemName,
                PCWSTR VolumeLabel,
                BOOL QuickFormat,
                DWORD ClusterSize,
                PFMIFSCALLBACK Callback
                );

            *(void**)&FormatEx = GetProcAddress(hmod, "FormatEx");

            if (FormatEx)
            {
                TlsSetValue(g_dwTlsIndex, &fd);
                FormatEx(driveRoot, FMIFS_MEDIA_TYPE::RemovableMedia,fileSystem, volumeLabel, TRUE, clusterSize, &FormatCallback);
            }

            FreeLibrary(hmod);
        }
    }

    return fd.fOk;
}

BOOL VolumeFormatter::Large_FAT32_Format(LPCSTR driveRoot)
{
    return formatLarge_FAT32(driveRoot);
}

void VolumeFormatter::formatDrive(const Drive& d , const std::wstring& targetFS)
{
    auto driveRoot = d.get_drivePath();
    std::basic_string<TCHAR> drivePath;
    drivePath += driveRoot;
    drivePath += _T(":\\");

    if (targetFS == L"NTFS")
    {
       auto clusterSize = getClusterSize( 15518924800 ,targetFS);
       FMIFS_Format(drivePath.c_str(), targetFS.c_str(), d.get_driveName().c_str(), clusterSize);
         
    }
    //use c.str() when sending to API!
}
DWORD VolumeFormatter::getClusterSize(uint64_t targetSize, const std::wstring& targetFS)
{
    //LOOKS DISGUSTING! PLEASE DIVIDE INTO SUB FUNCTIONS

    // Convert target size from MB to bytes
   // uint64_t targetSizeBytes = targetSizeMB * MB;

    if (targetFS == L"FAT32")
    {
        if (targetSize < 64ull * MB)
        {
            return 512; // bytes
        }
        else if (targetSize <= 128ull * MB)
        {
            return 1024; // bytes
        }
        else if (targetSize <= 256ull * MB)
        {
            return 2048; // bytes
        }
        else if (targetSize <= 8192ull * MB)
        {
            return 4096; // bytes
        }
        else if (targetSize <= 16384ull * MB)
        {
            return 8192; // bytes
        }
        else if (targetSize <= 32768ull * MB)
        {
            return 16384; // bytes
        }
        else
        {
            return 32768; // bytes
        }
    }
    else if (targetFS == L"exFAT")
    {
        if (targetSize < 7ull * MB)
        {
            return 4096; // bytes
        }
        else if (targetSize <= 256ull * MB)
        {
            return 32768; // bytes
        }
        else if (targetSize <= 32768ull * MB)
        {
            return 131072; // bytes
        }
        else
        {
            return 0; // Not supported
        }
    }
    else if (targetFS == L"NTFS")
    {
        https://techcommunity.microsoft.com/t5/storage-at-microsoft/cluster-size-recommendations-for-refs-and-ntfs/ba-p/425960
        return 4096; // bytes  
    }
    else
    {
        // Default case or error handling
        return 0;
    }
}
