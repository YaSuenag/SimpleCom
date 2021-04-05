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
#pragma once

#include "stdafx.h"

/*
 * Exception class for Windows API error.
 * This class can hold error code from GetLastError() and caption string for its error.
 * Also it can provides error string from FormatMessage(). Its pointer would be managed by WinAPIException.
 */
class WinAPIException
{
private:
	DWORD _error_code;
	LPCTSTR _error_caption;
	LPTSTR _error_text;

public:
	explicit WinAPIException(DWORD error_code) : WinAPIException(error_code, _T("SimpleCom")) {}
	WinAPIException(DWORD error_code, LPCTSTR error_caption);
	virtual ~WinAPIException();

	inline DWORD GetErrorCode() const noexcept {
		return _error_code;
	}

	inline LPCTSTR GetErrorCaption() const noexcept {
		return _error_caption;
	}

	inline LPCTSTR GetErrorText() const noexcept {
		return _error_text;
	}
};

