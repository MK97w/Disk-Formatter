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


BOOL VolumeFormatter::TryFormat()
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
                FormatEx(L"D:\\", FMIFS_MEDIA_TYPE::RemovableMedia, L"NTFS", L"REMAIN_INDOORS", TRUE, 8192, &FormatCallback);
            }

            FreeLibrary(hmod);
        }
    }

    return fd.fOk;
}