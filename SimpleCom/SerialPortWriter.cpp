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
#include "stdafx.h"

#include "SerialPortWriter.h"
#include "WinAPIException.h"

SerialPortWriter::SerialPortWriter(const HANDLE handle, DWORD buf_sz)
{
	_handle = handle;
	_overlapped = { 0 };

	_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (_overlapped.hEvent == NULL) {
		throw WinAPIException(GetLastError(), _T("SimpleCom"));
	}
	if (!SetEvent(_overlapped.hEvent)) {
		throw WinAPIException(GetLastError(), _T("SimpleCom"));
	}

	_buf_sz = buf_sz;
	_buf = new char[buf_sz];
	_buf_idx = 0;
}

SerialPortWriter::~SerialPortWriter()
{
	WriteAsync();
	WaitForSingleObject(_overlapped.hEvent, INFINITE);
	CloseHandle(_overlapped.hEvent);
	delete[] _buf;
}

void SerialPortWriter::WriteAsync()
{
	if (_buf_idx <= 0) {
		// No data to write
		return;
	}

	// Wait if async writing is performing...
	WaitForSingleObject(_overlapped.hEvent, INFINITE);

	ResetEvent(_overlapped.hEvent);
	BOOL result = WriteFile(_handle, _buf, _buf_idx, nullptr, &_overlapped);
	DWORD last_error = GetLastError();

	if (!result && (last_error != ERROR_IO_PENDING)) {
		throw WinAPIException(last_error, _T("SimpleCom"));
	}

	_buf_idx = 0;
}

void SerialPortWriter::Put(const char c) {
	// Wait if async writing is performing...
	WaitForSingleObject(_overlapped.hEvent, INFINITE);

	_buf[_buf_idx++] = c;

	if (_buf_idx == _buf_sz) {
		// If buffer is full, it would be sent on serial port.
		WriteAsync();
	}

}