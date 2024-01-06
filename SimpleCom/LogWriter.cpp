/*
 * Copyright (C) 2024, Yasumasa Suenaga
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
#include "LogWriter.h"
#include "WinAPIException.h"
#include "debug.h"


SimpleCom::LogWriter::LogWriter(LPCTSTR logfilename) {
	_handle = CreateFile(logfilename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, nullptr);
	if (_handle == INVALID_HANDLE_VALUE) {
		throw WinAPIException(GetLastError());
	}
	if (SetFilePointer(_handle, 0L, nullptr, FILE_END) == INVALID_SET_FILE_POINTER) {
		throw WinAPIException(GetLastError());
	}
}

SimpleCom::LogWriter::~LogWriter() {
	CloseHandle(_handle);
}

void SimpleCom::LogWriter::Write(const char c) {
	Write(&c, 1);
}

void SimpleCom::LogWriter::Write(const char* data, const DWORD len) {
	DWORD nBytesWritten;
	BOOL result = WriteFile(_handle, data, len, &nBytesWritten, nullptr);
	if (!result) {
		throw WinAPIException(GetLastError());
	}
	if (nBytesWritten != len) {
		SimpleCom::debug::log(_T("(Part of) log data could not be written."));
	}
}