/*
 * Copyright (C) 2019, 2021, Yasumasa Suenaga
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
#include "stdafx.h"

#include "WinAPIException.h"
#include "debug.h"

static thread_local TCHAR fallback_error_text[100];


SimpleCom::WinAPIException::WinAPIException(DWORD error_code, LPCTSTR error_caption) : _error_code(error_code),
                                                                                       _error_caption(error_caption),
	                                                                                   _error_text(nullptr)
{
	DWORD result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, _error_code, 0, reinterpret_cast<LPTSTR>(&_error_text), 0, NULL);
	if (result == 0) {
		DWORD errcode = GetLastError();
		_sntprintf_s(fallback_error_text, sizeof(fallback_error_text) / sizeof(TCHAR), _T("Error occured (%#x)"), _error_code);
		_error_text = fallback_error_text;

		TStringStream msg;
		msg << _T("Error occurred in FormatMessage (") << std::showbase << std::hex << errcode << _T(")");
		debug::log(msg.str().c_str());
	}
}


SimpleCom::WinAPIException::~WinAPIException()
{
	if ((_error_text != nullptr) && (_error_text != fallback_error_text) && (_error_code != -1)) {
		LocalFree(_error_text);
	}
}
