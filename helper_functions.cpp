#include "helper_functions.h"

namespace helperFunction
{

    std::wstring _API_CompatablePath(TCHAR tchar)
    {
        std::wstring path = L"\\\\.\\";
        path += tchar;
        path += L":";
        return path;
    }

    uint16_t _nextPowerOfTwo(uint16_t val)
    {
        val--;
        val |= val >> 1;
        val |= val >> 2;
        val |= val >> 4;
        val |= val >> 8;
        val++;
        return val;
    }

    std::string addSuffix(int suffix, std::string str)
    {
        if (suffix == 1)
            str += " KB";
        else if (suffix == 2)
            str += " MB";
        else if (suffix == 3)
            str += " GB";
        else if (suffix == 4)
            str += " TB";

        return str;
    }


}