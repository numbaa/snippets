#include <Windows.h>

#include <iostream>
#include <string>
#include <vector>

std::wstring getProgramFullpath() {
    std::vector<wchar_t> fullpath(MAX_PATH);
    DWORD length = ::GetModuleFileNameW(nullptr, fullpath.data(), MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::wstring(fullpath.data(), length);
    }
    return L"";
}

std::wstring getProgramPath() {
    std::wstring fullpath = getProgramFullpath();
    if (fullpath.size() != 0) {
        std::wstring::size_type pos = fullpath.rfind(L'\\');
        return fullpath.substr(0, pos);
    }
    return L"";
}


std::wstring getProgramName() {
    std::wstring fullpath = getProgramFullpath();
    if (fullpath.size() != 0) {
        std::wstring::size_type pos = fullpath.rfind(L'\\');
        if (pos != std::wstring::npos) {
            return fullpath.substr(pos + 1);
        }
    }
    return L"";
}

int main()
{
    std::wcout << "Program path: " << getProgramPath() << std::endl;
    std::wcout << "Program full path: " << getProgramFullpath() << std::endl;
    std::wcout << "Program name: " << getProgramName() << std::endl;
    return 0;
}