#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <string>
#include <sstream>
#include <windows.h>

namespace helperFunction
{
    template <typename T>
    std::string _toString(T val)
    {
        std::stringstream stream;
        stream << val;
        return stream.str();
    }

    std::wstring _API_CompatablePath(TCHAR tchar);

    uint16_t _nextPowerOfTwo(uint16_t val);

    std::string addSuffix(int suffix, std::string str);
}

#endif // HELPER_FUNCTION_H