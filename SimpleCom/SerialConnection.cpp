/*
 * Copyright (C) 2023, 2024, Yasumasa Suenaga
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
#include "SerialConnection.h"
#include "util.h"
#include "debug.h"
#include "../common/common.h"

static constexpr int buf_sz = 256;

typedef struct {
	HANDLE hSerial;
	OVERLAPPED *overlapped;
	HANDLE hTermEvent;
	HWND parent_hwnd;
	HANDLE hStdOut;
} TStdOutRedirectorParam;


SimpleCom::SerialConnection::SerialConnection(TString& device, DCB* dcb, HWND hwnd, HANDLE hStdIn, HANDLE hStdOut, bool useTTYResizer) :
	_device(device),
	_parent_hwnd(hwnd),
	_hStdIn(hStdIn),
	_hStdOut(hStdOut),
	_useTTYResizer(useTTYResizer)
{
	CopyMemory(&_dcb, dcb, sizeof(_dcb));
}

void SimpleCom::SerialConnection::InitSerialPort(const HANDLE hSerial) {
	TString title = _T("SimpleCom: ") + _device;
	CALL_WINAPI_WITH_DEBUGLOG(SetConsoleTitle(title.c_str()), TRUE, __FILE__, __LINE__)

	CALL_WINAPI_WITH_DEBUGLOG(SetCommState(hSerial, &_dcb), TRUE, __FILE__, __LINE__)

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

/*
 * Write key code in KEY_EVENT_RECORD to send buffer.
 * If F1 key is found, all of send_data would be flushed and would set true to terminated flag.
 * Return true if F1 key is pushed (active close).
 */
bool SimpleCom::SerialConnection::ProcessKeyEvents(const KEY_EVENT_RECORD keyevent, SerialPortWriter& writer, const HANDLE hTermEvent) {

	if (keyevent.wVirtualKeyCode == VK_F1) {
		// Write all keys in the buffer before F1
		writer.WriteAsync();

		if (MessageBox(_parent_hwnd, _T("Do you want to leave from this serial session?"), _T("SimpleCom"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
			SetEvent(hTermEvent);
			return true;
		}

		// Return immediately without issueing escape sequence of F1
		return false;
	}

	if (keyevent.bKeyDown && (keyevent.uChar.AsciiChar != '\0')) {
		for (int send_idx = 0; send_idx < keyevent.wRepeatCount; send_idx++) {
			writer.Put(keyevent.uChar.AsciiChar);
		}
	}

	return false;
}

static void StdOutRedirectorLoopInner(const HANDLE hSerial, OVERLAPPED *overlapped, const HANDLE hStdOut) {
	DWORD event_mask = 0;
	if (!WaitCommEvent(hSerial, &event_mask, overlapped)) {
		if (GetLastError() == ERROR_IO_PENDING) {
			DWORD unused = 0;
			if (!GetOverlappedResult(hSerial, overlapped, &unused, TRUE)) {
				throw SimpleCom::WinAPIException(GetLastError(), _T("GetOverlappedResult for WaitCommEvent"));
			}
		}
		else {
			throw SimpleCom::WinAPIException(GetLastError(), _T("WaitCommEvent"));
		}
	}

	if (event_mask & EV_RXCHAR) {
		DWORD errors;
		COMSTAT comstat = { 0 };
		if (!ClearCommError(hSerial, &errors, &comstat)) {
			throw SimpleCom::WinAPIException(GetLastError(), _T("ClearCommError"));
		}

		char buf[buf_sz] = { 0 };
		DWORD nBytesRead = 0;
		DWORD remainBytes = comstat.cbInQue;
		while (remainBytes > 0) {

			if (!ReadFile(hSerial, buf, min(buf_sz, remainBytes), &nBytesRead, overlapped)) {
				if (GetLastError() == ERROR_IO_PENDING) {
					if (!GetOverlappedResult(hSerial, overlapped, &nBytesRead, FALSE)) {
						throw SimpleCom::WinAPIException(GetLastError(), _T("GetOverlappedResult for ReadFile"));
					}
				}
				else {
					throw SimpleCom::WinAPIException(GetLastError(), _T("ReadFile from serial device"));
				}
			}

			if (nBytesRead > 0) {
				DWORD nBytesWritten;
				if (!WriteFile(hStdOut, buf, nBytesRead, &nBytesWritten, NULL)) {
					throw SimpleCom::WinAPIException(GetLastError(), _T("WriteFile to stdout"));
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
	TStdOutRedirectorParam* param = reinterpret_cast<TStdOutRedirectorParam*>(lpParameter);

	while (WaitForSingleObject(param->hTermEvent, 0) != WAIT_OBJECT_0) {
		try {

			if (!ResetEvent(param->overlapped->hEvent)) {
				throw SimpleCom::WinAPIException(GetLastError(), _T("ResetEvent for reading data from serial device"));
			}

			StdOutRedirectorLoopInner(param->hSerial, param->overlapped, param->hStdOut);
		}
		catch (SimpleCom::WinAPIException& e) {
			if (WaitForSingleObject(param->hTermEvent, 0) != WAIT_OBJECT_0) {
				SetEvent(param->hTermEvent);
				if (e.GetErrorCode() == ERROR_OPERATION_ABORTED) {
					// We can ignore ERROR_OPERATION_ABORTED because it would be intended.
					return 0;
				}
				else {
					MessageBox(param->parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
					return -1;
				}
			}
		}
	}

	return 0;
}

/*
 * Entry point for stdin redirector.
 * stdin redirects stdin to serial (write op).
 * Return true if F1 key is pushed (active close).
 */
bool SimpleCom::SerialConnection::StdInRedirector(const HANDLE hSerial, const HANDLE hTermEvent) {
	INPUT_RECORD inputs[buf_sz];
	DWORD n_read;

	SimpleCom::SerialPortWriter writer(hSerial, buf_sz);

	try {
		HANDLE waiters[] = { _hStdIn, hTermEvent };
		while (true) {
			DWORD result = WaitForMultipleObjects(sizeof(waiters) / sizeof(HANDLE), waiters, FALSE, INFINITE);
			if (result == WAIT_OBJECT_0) { // hStdIn
				if (!ReadConsoleInput(_hStdIn, inputs, sizeof(inputs) / sizeof(INPUT_RECORD), &n_read)) {
					throw SimpleCom::WinAPIException(GetLastError());
				}

				for (DWORD idx = 0; idx < n_read; idx++) {
					if (inputs[idx].EventType == KEY_EVENT) {
						// Skip escape sequence of F1 (0x1B 0x4F 0x50)
						//
						// Input sequence on Windows (ENABLE_VIRTUAL_TERMINAL_INPUT):
						//   https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#numpad--function-keys
						if (idx + 2 < n_read &&
							(inputs[idx].Event.KeyEvent.wRepeatCount == 1 && inputs[idx].Event.KeyEvent.uChar.AsciiChar == '\x1b' /* ESC */) &&
							(inputs[idx + 1].Event.KeyEvent.wRepeatCount == 1 && inputs[idx + 1].Event.KeyEvent.uChar.AsciiChar == 'O') &&
							(inputs[idx + 2].Event.KeyEvent.wRepeatCount == 1 && inputs[idx + 2].Event.KeyEvent.uChar.AsciiChar == 'P')) {
							idx += 2;
							continue;
						}

						if (ProcessKeyEvents(inputs[idx].Event.KeyEvent, writer, hTermEvent)) {
							return true;
						}
					}
					else if ((inputs[idx].EventType == WINDOW_BUFFER_SIZE_EVENT) && _useTTYResizer) {
						char buf[RINGBUF_SZ];
						int len = snprintf(buf, sizeof(buf), "%c%d" RESIZER_SEPARATOR "%d%c", RESIZER_START_MARKER, inputs[idx].Event.WindowBufferSizeEvent.dwSize.Y, inputs[idx].Event.WindowBufferSizeEvent.dwSize.X, RESIZER_END_MARKER);
						writer.PutData(buf, len);
					}
				}

				writer.WriteAsync();
			}
			else if (result == (WAIT_OBJECT_0 + 1)) { // hTermEvent
				break;
			}
			else {
				throw SimpleCom::WinAPIException(GetLastError(), _T("WaitForMultipleObjects in StdInRedirector"));
			}
		}
	}
	catch (SimpleCom::WinAPIException& e) {
		// Fire terminate event before MessageBox is shown
		// because other threads should be terminated immediately.
		SetEvent(hTermEvent);
		// We can ignore ERROR_OPERATION_ABORTED because it would be intended.
		if (e.GetErrorCode() != ERROR_OPERATION_ABORTED) {
			MessageBox(_parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		}
	}

	writer.Shutdown();
	return false;
}

/*
 * Talk with peripheral.
 * Return true if F1 key is pushed (active close).
 */
bool SimpleCom::SerialConnection::DoSession() {
	HandleHandler hSerial(CreateFile(_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL), _T("Open serial port"));
	InitSerialPort(hSerial.handle());

	HandleHandler hIoEvent(CreateEvent(NULL, TRUE, TRUE, NULL), _T("CreateEvent for reading from serial device"));
	HandleHandler hTermEvent(CreateEvent(NULL, TRUE, FALSE, NULL), _T("CreateEvent for thread termination"));
	OVERLAPPED serialReadOverlapped = { 0 };
	serialReadOverlapped.hEvent = hIoEvent.handle();

	// Create stdout redirector
	TStdOutRedirectorParam param = {
		.hSerial = hSerial.handle(),
		.overlapped = &serialReadOverlapped,
		.hTermEvent = hTermEvent.handle(),
		.parent_hwnd = this->_parent_hwnd,
		.hStdOut = this->_hStdOut
	};
	HandleHandler threadHnd(CreateThread(NULL, 0, &StdOutRedirector, &param, 0, NULL), _T("CreateThread for StdOutRedirector"));

	// stdin redirector would perform in current thread
	bool exited = StdInRedirector(hSerial.handle(), hTermEvent.handle());

	// stdin redirector should be finished at this point.
	// It means end of serial communication. So we should terminate stdout redirector.
	CALL_WINAPI_WITH_DEBUGLOG(CancelIoEx(hSerial.handle(), &serialReadOverlapped), TRUE, __FILE__, __LINE__)
	WaitForSingleObject(threadHnd.handle(), INFINITE);

	return exited;
}