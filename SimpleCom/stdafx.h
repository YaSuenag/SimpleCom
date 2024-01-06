#pragma once

// Windows Version
#include <winsdkver.h>
#ifndef WINVER
#define WINVER _WIN32_WINNT_WINBLUE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINBLUE
#endif
#include <sdkddkver.h>

// System Includes
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <commdlg.h>

#include <tchar.h>

#include <iostream>
#include <map>
#include <vector>
#include <regex>
#include <sstream>
#include <string>
#include <codecvt>

#ifdef _UNICODE
typedef std::wstring TString;
typedef std::wstringstream TStringStream;
typedef std::wregex TRegex;
#else
typedef std::string TString;
typedef std::stringstream TStringStream;
typedef std::regex TRegex;
#endif