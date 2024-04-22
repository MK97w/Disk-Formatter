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