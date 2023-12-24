#pragma once
#include <iostream>
#include <codecvt>


#define ERROR_LOG(...){printf(__VA_ARGS__); printf("\n");fflush(stdout);}
#define DEBUG_LOG(...){printf(__VA_ARGS__); printf("\n");fflush(stdout);}


inline std::string WcharToChar(std::wstring const& wstr)
{

    //setup converter
    using convert_type = std::codecvt_utf8_utf16<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;

    //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
    std::string converted_str = converter.to_bytes(wstr);

    return converted_str;
}

inline std::wstring CharToWchar(std::string const& str)
{
    //setup converter
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(str);

    return wide;
}

inline std::string_view getFileNameFromPath(std::string_view fileName) {
    int var = fileName.find_last_of('\\');
    if (var == std::string::npos && var < fileName.size() - 1)
        return "";
    return fileName.substr(var + 1);
}

inline std::string getFileNameFromPath(const char* fileName) {
    return std::string(getFileNameFromPath(std::string(fileName)));
}

/*
std::string WcharToChar(std::wstring const& wstr)
{
    std::size_t size = sizeof(wstr.c_str());
    char* str = new char[size];
    std::string temp;

    std::wcstombs(str, wstr.c_str(), size);

    temp = str;
    delete[] str;

    return temp;
}

std::wstring CharToWchar(std::string const& str)
{
    std::size_t size = sizeof(str.c_str());
    wchar_t* wstr = new wchar_t[size];
    std::wstring temp;

    std::mbstowcs(wstr, str.c_str(), size);

    temp = wstr;
    delete[] wstr;

    return temp;
}
*/