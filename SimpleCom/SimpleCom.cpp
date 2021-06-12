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

#include <iostream>

#include "SerialSetup.h"
#include "SerialPortWriter.h"
#include "WinAPIException.h"

static HANDLE stdoutRedirectorThread;

static HANDLE hSerial;
static OVERLAPPED serialReadOverlapped = { 0 };
static volatile bool terminated;

static constexpr int buf_sz = 256;


/*
 * Write key code in KEY_EVENT_RECORD to send buffer.
 * If F1 key is found, all of send_data would be flushed and would set true to terminated flag.
 * Return the index of send_data to write next one.
 */
static void ProcessKeyEvents(const KEY_EVENT_RECORD keyevent, SerialPortWriter &writer, const HWND parent_hwnd) {

	if (keyevent.wVirtualKeyCode == VK_F1) {
		// Write all keys in the buffer before F1
		writer.WriteAsync();

		if (MessageBox(parent_hwnd, _T("Do you want to leave from this serial session?"), _T("SimpleCom"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
			terminated = true;
			return;
		}
	}

	if (keyevent.bKeyDown) {
		for (int send_idx = 0; send_idx < keyevent.wRepeatCount; send_idx++) {
			writer.Put(keyevent.uChar.AsciiChar);
		}
	}

}

/*
 * Entry point for stdin redirector.
 * stdin redirects stdin to serial (write op).
 */
static void StdInRedirector(HWND parent_hwnd) {
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD inputs[buf_sz];
	DWORD n_read;

	SerialPortWriter writer(hSerial, buf_sz);

	try {
		while (!terminated) {

			if (!ReadConsoleInput(hStdIn, inputs, sizeof(inputs) / sizeof(INPUT_RECORD), &n_read)) {
				throw WinAPIException(GetLastError(), _T("SimpleCom"));
			}

			for (DWORD idx = 0; idx < n_read; idx++) {
				if (inputs[idx].EventType == KEY_EVENT) {
					ProcessKeyEvents(inputs[idx].Event.KeyEvent, writer, parent_hwnd);
				}
			}

			writer.WriteAsync();
		}
	}
	catch (WinAPIException e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
	}

	terminated = true;
}

/*
 * Entry point for stdout redirector.
 * stdout redirects serial (read op) to stdout.
 */
DWORD WINAPI StdOutRedirector(_In_ LPVOID lpParameter) {
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	char buf[buf_sz] = { 0 };
	DWORD nBytesRead = 0;

	while (!terminated) {
		ResetEvent(serialReadOverlapped.hEvent);
		DWORD event_mask = 0;
		if (!WaitCommEvent(hSerial, &event_mask, &serialReadOverlapped)) {
			if (GetLastError() == ERROR_IO_PENDING) {
				if (!GetOverlappedResult(hSerial, &serialReadOverlapped, &nBytesRead, TRUE)) {
					return -1;
				}
			}
		}

		if (event_mask & EV_RXCHAR) {
			DWORD errors;
			COMSTAT comstat = { 0 };
			if (!ClearCommError(hSerial, &errors, &comstat)) {
				return -1;
			}

			DWORD remainBytes = comstat.cbInQue;
			while (remainBytes > 0) {

				if (!ReadFile(hSerial, buf, min(buf_sz, remainBytes), &nBytesRead, &serialReadOverlapped)) {
					if (GetLastError() == ERROR_IO_PENDING) {
						if (!GetOverlappedResult(hSerial, &serialReadOverlapped, &nBytesRead, FALSE)) {
							return -1;
						}
					}
				}

				if (nBytesRead > 0) {
					DWORD nBytesWritten;
					WriteFile(hStdOut, buf, nBytesRead, &nBytesWritten, NULL);
					remainBytes -= nBytesRead;
				}

			}

		}


	}

	return 0;
}

static HWND GetParentWindow() {
	HWND current = GetConsoleWindow();

	while (true) {
		HWND parent = GetParent(current);

		if (parent == NULL) {
			TCHAR window_text[MAX_PATH] = { 0 };
			GetWindowText(current, window_text, MAX_PATH);

			// If window_text is empty, SimpleCom might be run on Windows Terminal (Could not get valid owner window text)
			return (window_text[0] == 0) ? NULL : current;
		}
		else {
			current = parent;
		}

	}

}

int _tmain(int argc, LPCTSTR argv[])
{
	DCB dcb;
	TString device;
	HWND parent_hwnd = GetParentWindow();

	// Serial port configuration
	try {
		SerialSetup setup;
		if (argc > 1) {
			// command line mode
			setup.ParseArguments(argc, argv);
		}
		else if (!setup.ShowConfigureDialog(NULL, parent_hwnd)) {
			return -1;
		}
		device = _T("\\\\.\\") + setup.GetPort();
		setup.SaveToDCB(&dcb);
	}
	catch (WinAPIException e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -1;
	}
	catch (SerialSetupException e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -2;
	}

	// Open serial device
	hSerial = CreateFile(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hSerial == INVALID_HANDLE_VALUE) {
		WinAPIException e(GetLastError(), NULL); // Use WinAPIException to get error string.
		MessageBox(parent_hwnd, e.GetErrorText(), _T("Open serial connection"), MB_OK | MB_ICONERROR);
		return -4;
	}

	TString title = _T("SimpleCom: ") + device;
	SetConsoleTitle(title.c_str());

	SetCommState(hSerial, &dcb);
	PurgeComm(hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	SetCommMask(hSerial, EV_RXCHAR);
	SetupComm(hSerial, buf_sz, buf_sz);

	COMMTIMEOUTS comm_timeouts;
	GetCommTimeouts(hSerial, &comm_timeouts);
	comm_timeouts.ReadIntervalTimeout = 0;
	comm_timeouts.ReadTotalTimeoutMultiplier = 0;
	comm_timeouts.ReadTotalTimeoutConstant = 10;
	comm_timeouts.WriteTotalTimeoutMultiplier = 0;
	comm_timeouts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(hSerial, &comm_timeouts);

	serialReadOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (serialReadOverlapped.hEvent == NULL) {
		WinAPIException ex(GetLastError(), _T("SimpleCom"));
		MessageBox(parent_hwnd, ex.GetErrorText(), ex.GetErrorCaption(), MB_OK | MB_ICONERROR);
		CloseHandle(hSerial);
		return -1;
	}

	DWORD mode;

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(hStdIn, &mode);
	mode &= ~ENABLE_PROCESSED_INPUT;
	mode &= ~ENABLE_LINE_INPUT;
	mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	SetConsoleMode(hStdIn, mode);

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(hStdOut, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(hStdOut, mode);

	terminated = false;

	// Create stdout redirector
	stdoutRedirectorThread = CreateThread(NULL, 0, &StdOutRedirector, NULL, 0, NULL);
	if (stdoutRedirectorThread == NULL) {
		WinAPIException ex(GetLastError(), _T("SimpleCom"));
		MessageBox(parent_hwnd, ex.GetErrorText(), ex.GetErrorCaption(), MB_OK | MB_ICONERROR);
		CloseHandle(hSerial);
		return -2;
	}

	// stdin redirector would perform in current thread
	StdInRedirector(parent_hwnd);

	// stdin redirector should be finished at this point.
	// It means end of serial communication. So we should terminate stdout redirector.
	CancelIoEx(hSerial, &serialReadOverlapped);
	WaitForSingleObject(stdoutRedirectorThread, INFINITE);

	CloseHandle(hSerial);
	CloseHandle(serialReadOverlapped.hEvent);

	return 0;
}
