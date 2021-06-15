#pragma once

// Windows Version
#include <winsdkver.h>
#define WINVER _WIN32_WINNT_WINBLUE
#define _WIN32_WINNT _WIN32_WINNT_WINBLUE
#include <sdkddkver.h>

// System Includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <tchar.h>

#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

#ifdef _UNICODE
typedef std::wstring TString;
typedef std::wstringstream TStringStream;
typedef std::wregex TRegex;
#else
typedef std::string TString;
typedef std::stringstream TStringStream;
typedef std::regex TRegex;
#endif