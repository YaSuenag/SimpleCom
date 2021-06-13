/*
 * Copyright (C) 2021, Yasumasa Suenaga
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#pragma once

#include "stdafx.h"

#ifdef _UNICODE
#define SS_CLASS std::wstringstream
#else
#define SS_CLASS std::stringstream
#endif

#define CALL_WINAPI_WITH_DEBUGLOG(func_call, expected, file, line)              \
  if (func_call != expected) {                                                  \
    WinAPIException e(GetLastError(), nullptr);                                 \
    SS_CLASS ss;                                                                \
    ss << file << _T(":") << line << _T(": ") << e.GetErrorText() << std::endl; \
    log_debug(ss.str().c_str());                                                \
  }

 // Print debug message to the console if the binary is debug build
void log_debug(LPCTSTR message);