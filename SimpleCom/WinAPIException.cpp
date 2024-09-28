/*
 * Copyright (C) 2019, 2024, Yasumasa Suenaga
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


SimpleCom::WinAPIException::WinAPIException(DWORD error_code, LPCTSTR error_caption) : _error_code(error_code),
                                                                                       _error_caption(error_caption)
{
	LPTSTR buf = nullptr;
	TStringStream error_msg;
	DWORD result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, _error_code, 0, reinterpret_cast<LPTSTR>(&buf), 0, NULL);
	if (result == 0) {
		DWORD errcode = GetLastError();
		error_msg << _T("Error occured (") << std::showbase << std::hex << _error_code << _T(")");

		TStringStream msg;
		msg << _T("Error occurred in FormatMessage (") << std::showbase << std::hex << errcode << _T(")");
		debug::log(msg.str().c_str());
	}
	else {
		error_msg << buf << _T(" (") << std::showbase << std::hex << _error_code << _T(")");
		LocalFree(buf);
	}
	_error_text = error_msg.str();
}