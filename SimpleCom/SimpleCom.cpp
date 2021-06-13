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

#include "SerialSetup.h"
#include "SerialPortWriter.h"
#include "WinAPIException.h"
#include "debug.h"

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
	catch (WinAPIException& e) {
		if (!terminated) {
			MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		}
	}

	terminated = true;
}

static void StdOutRedirectorLoopInner(HANDLE hStdOut) {
	DWORD event_mask = 0;
	if (!WaitCommEvent(hSerial, &event_mask, &serialReadOverlapped)) {
		if (GetLastError() == ERROR_IO_PENDING) {
			DWORD unused = 0;
			if (!GetOverlappedResult(hSerial, &serialReadOverlapped, &unused, TRUE)) {
				throw WinAPIException(GetLastError(), _T("GetOverlappedResult for WaitCommEvent"));
			}
		}
		else {
			throw WinAPIException(GetLastError(), _T("WaitCommEvent"));
		}
	}

	if (event_mask & EV_RXCHAR) {
		DWORD errors;
		COMSTAT comstat = { 0 };
		if (!ClearCommError(hSerial, &errors, &comstat)) {
			throw WinAPIException(GetLastError(), _T("ClearCommError"));
		}

		char buf[buf_sz] = { 0 };
		DWORD nBytesRead = 0;
		DWORD remainBytes = comstat.cbInQue;
		while (remainBytes > 0) {

			if (!ReadFile(hSerial, buf, min(buf_sz, remainBytes), &nBytesRead, &serialReadOverlapped)) {
				if (GetLastError() == ERROR_IO_PENDING) {
					if (!GetOverlappedResult(hSerial, &serialReadOverlapped, &nBytesRead, FALSE)) {
						throw WinAPIException(GetLastError(), _T("GetOverlappedResult for ReadFile"));
					}
				}
				else {
					throw WinAPIException(GetLastError(), _T("ReadFile from serial device"));
				}
			}

			if (nBytesRead > 0) {
				DWORD nBytesWritten;
				if (!WriteFile(hStdOut, buf, nBytesRead, &nBytesWritten, NULL)) {
					throw WinAPIException(GetLastError(), _T("WriteFile to stdout"));
				}
				remainBytes -= nBytesRead;
			}

		}

	}
}

/*
 * Entry point for stdout redirector.
 * stdout redirects serial (read op) to stdout.
 */
DWORD WINAPI StdOutRedirector(_In_ LPVOID lpParameter) {
	HWND parent_hwnd = reinterpret_cast<HWND>(lpParameter);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	while (!terminated) {
		try {

			if (!ResetEvent(serialReadOverlapped.hEvent)) {
				throw WinAPIException(GetLastError(), _T("ResetEvent for reading data from serial device"));
			}

			StdOutRedirectorLoopInner(hStdOut);
		}
		catch (WinAPIException& e) {
			if (!terminated) {
				MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
				terminated = true;
				return -1;
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
			int result = GetWindowText(current, window_text, MAX_PATH);

			// If window_text is empty, SimpleCom might be run on Windows Terminal (Could not get valid owner window text)
			return ((result == 0) || (window_text[0] == 0)) ? NULL : current;
		}
		else {
			current = parent;
		}

	}

}

static void InitSerialPort(TString &device, DCB *dcb) {
	TString title = _T("SimpleCom: ") + device;
	CALL_WINAPI_WITH_DEBUGLOG(SetConsoleTitle(title.c_str()), TRUE, __FILE__, __LINE__)

	CALL_WINAPI_WITH_DEBUGLOG(SetCommState(hSerial, dcb), TRUE, __FILE__, __LINE__)

	CALL_WINAPI_WITH_DEBUGLOG(PurgeComm(hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR), TRUE, __FILE__, __LINE__)
	CALL_WINAPI_WITH_DEBUGLOG(SetCommMask(hSerial, EV_RXCHAR), TRUE, __FILE__, __LINE__)
	CALL_WINAPI_WITH_DEBUGLOG(SetupComm(hSerial, buf_sz, buf_sz), TRUE, __FILE__, __LINE__)

	COMMTIMEOUTS comm_timeouts;
	CALL_WINAPI_WITH_DEBUGLOG(GetCommTimeouts(hSerial, &comm_timeouts), TRUE, __FILE__, __LINE__)
	comm_timeouts.ReadIntervalTimeout = 0;
	comm_timeouts.ReadTotalTimeoutMultiplier = 0;
	comm_timeouts.ReadTotalTimeoutConstant = 10;
	comm_timeouts.WriteTotalTimeoutMultiplier = 0;
	comm_timeouts.WriteTotalTimeoutConstant = 0;
	CALL_WINAPI_WITH_DEBUGLOG(SetCommTimeouts(hSerial, &comm_timeouts), TRUE, __FILE__, __LINE__)
}

static void InitConsole(HANDLE *hStdIn, HANDLE *hStdOut) {
	DWORD mode;

	*hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	if (*hStdIn == INVALID_HANDLE_VALUE) {
		throw WinAPIException(GetLastError(), _T("GetStdHandle(stdin)"));
	}
	CALL_WINAPI_WITH_DEBUGLOG(GetConsoleMode(*hStdIn, &mode), TRUE, __FILE__, __LINE__)
	mode &= ~ENABLE_PROCESSED_INPUT;
	mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	CALL_WINAPI_WITH_DEBUGLOG(SetConsoleMode(*hStdIn, mode), TRUE, __FILE__, __LINE__)

	*hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (*hStdOut == INVALID_HANDLE_VALUE) {
		throw WinAPIException(GetLastError(), _T("GetStdHandle(stdout)"));
	}
	CALL_WINAPI_WITH_DEBUGLOG(GetConsoleMode(*hStdOut, &mode), TRUE, __FILE__, __LINE__);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	CALL_WINAPI_WITH_DEBUGLOG(SetConsoleMode(*hStdOut, mode), TRUE, __FILE__, __LINE__);
}

/*
 * RAII for HANDLE
 */
class HandleHandler {
private:
	HANDLE _handle;

public:
	HandleHandler(HANDLE handle, LPCTSTR error_caption) : _handle(handle) {
		if (_handle == INVALID_HANDLE_VALUE) {
			throw WinAPIException(GetLastError(), error_caption);
		}
	}

	~HandleHandler() {
		if (_handle != INVALID_HANDLE_VALUE) {
			CloseHandle(_handle);
		}
	}

	inline HANDLE handle() {
		return _handle;
	}
};

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
		device = _T(R"(\\.\)") + setup.GetPort();
		setup.SaveToDCB(&dcb);
	}
	catch (WinAPIException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -1;
	}
	catch (SerialSetupException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -2;
	}

	try {
		// Open serial device
		HandleHandler hnd(CreateFile(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL), _T("Open serial port"));
		hSerial = hnd.handle();
		InitSerialPort(device, &dcb);

		HandleHandler evt(CreateEvent(NULL, TRUE, TRUE, NULL), _T("CreateEvent for reading from serial device"));
		serialReadOverlapped.hEvent = evt.handle();

		HANDLE hStdIn, hStdOut;
		InitConsole(&hStdIn, &hStdOut);

		terminated = false;

		// Create stdout redirector
		HandleHandler threadHnd(CreateThread(NULL, 0, &StdOutRedirector, parent_hwnd, 0, NULL), _T("CreateThread for StdOutRedirector"));
		stdoutRedirectorThread = threadHnd.handle();

		// stdin redirector would perform in current thread
		StdInRedirector(parent_hwnd);

		// stdin redirector should be finished at this point.
		// It means end of serial communication. So we should terminate stdout redirector.
		CALL_WINAPI_WITH_DEBUGLOG(CancelIoEx(hSerial, &serialReadOverlapped), TRUE, __FILE__, __LINE__)
		WaitForSingleObject(stdoutRedirectorThread, INFINITE);
	}
	catch (WinAPIException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -3;
	}

	return 0;
}
