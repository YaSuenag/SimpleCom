/*
 * Copyright (C) 2025, Yasumasa Suenaga
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
#include "TerminalRedirector.h"
#include "SerialPortWriter.h"
#include "LogWriter.h"
#include "debug.h"
#include "WinAPIException.h"
#include "../common/common.h"

static constexpr LPCTSTR CLEAR_CONSOLE_COMMAND = _T("\x1b[2J");
static DWORD CLEAR_CONSOLE_COMMAND_LEN = static_cast<DWORD>(_tcslen(CLEAR_CONSOLE_COMMAND));


/*
 * Entry point for stdout redirector.
 * stdout redirects serial (read op) to stdout.
 */
DWORD WINAPI StdOutRedirector(_In_ LPVOID lpParameter) {
	SimpleCom::TStdOutRedirectorParam* param = reinterpret_cast<SimpleCom::TStdOutRedirectorParam*>(lpParameter);

	while (WaitForSingleObject(param->hTermEvent, 0) != WAIT_OBJECT_0) {
		try {

			if (!ResetEvent(param->overlapped.hEvent)) {
				throw SimpleCom::WinAPIException(GetLastError(), _T("ResetEvent for reading data from serial device"));
			}

			DWORD event_mask = 0;
			if (!WaitCommEvent(param->hSerial, &event_mask, &param->overlapped)) {
				if (GetLastError() == ERROR_IO_PENDING) {
					DWORD unused = 0;
					if (!GetOverlappedResult(param->hSerial, &param->overlapped, &unused, TRUE)) {
						throw SimpleCom::SerialAPIException(GetLastError(), _T("GetOverlappedResult for WaitCommEvent"));
					}
				}
				else {
					throw SimpleCom::SerialAPIException(GetLastError(), _T("WaitCommEvent"));
				}
			}

			if (event_mask & EV_RXCHAR) {
				DWORD errors;
				COMSTAT comstat = { 0 };
				if (!ClearCommError(param->hSerial, &errors, &comstat)) {
					throw SimpleCom::SerialAPIException(GetLastError(), _T("ClearCommError"));
				}

				char buf[buf_sz] = { 0 };
				DWORD nBytesRead = 0;
				DWORD remainBytes = comstat.cbInQue;
				while (remainBytes > 0) {

					if (!ReadFile(param->hSerial, buf, min(buf_sz, remainBytes), &nBytesRead, &param->overlapped)) {
						if (GetLastError() == ERROR_IO_PENDING) {
							if (!GetOverlappedResult(param->hSerial, &param->overlapped, &nBytesRead, FALSE)) {
								throw SimpleCom::SerialAPIException(GetLastError(), _T("GetOverlappedResult for ReadFile"));
							}
						}
						else {
							throw SimpleCom::SerialAPIException(GetLastError(), _T("ReadFile from serial device"));
						}
					}

					if (nBytesRead > 0) {
						DWORD nBytesWritten;
						if (!WriteFile(param->hStdOut, buf, nBytesRead, &nBytesWritten, NULL)) {
							throw SimpleCom::WinAPIException(GetLastError(), _T("WriteFile to stdout"));
						}
						if (param->logwriter != nullptr) {
							param->logwriter->Write(buf, nBytesRead);
						}
						remainBytes -= nBytesRead;
					}

				}

			}
		}
		catch (SimpleCom::WinAPIException& e) {
			if (WaitForSingleObject(param->hTermEvent, 0) != WAIT_OBJECT_0) {
				SetEvent(param->hTermEvent);
				param->exception_handler(e);
				break;
			}
		}
	}

	return 0;
}

/*
 * Ask user whether terminate current serial session via dialog box.
 * Return true if session should be closed (active close).
 */
static bool ShouldTerminate(HWND parent_hwnd, SimpleCom::SerialPortWriter& writer, const HANDLE hTermEvent) {
	// Write all keys in the buffer before F1
	writer.WriteAsync();

	if (MessageBox(parent_hwnd, _T("Do you want to leave from this serial session?"), _T("SimpleCom"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
		SetEvent(hTermEvent);
		return true;
	}

	return false;
}

/*
 * Write key code in KEY_EVENT_RECORD to send buffer.
 */
static void ProcessKeyEvents(const KEY_EVENT_RECORD keyevent, SimpleCom::SerialPortWriter& writer, SimpleCom::LogWriter *logwriter) {

	if (keyevent.wVirtualKeyCode == VK_F1) {
		// F1 key should not be propagated to peripheral.
		return;
	}

	if (keyevent.bKeyDown && (keyevent.uChar.AsciiChar != '\0')) {
		for (int send_idx = 0; send_idx < keyevent.wRepeatCount; send_idx++) {
			writer.Put(keyevent.uChar.AsciiChar);
			if(logwriter != nullptr) {
				// Write key to log file if logging is enabled.
				logwriter->Write(keyevent.uChar.AsciiChar);
			}
		}
	}
}

/*
 * Entry point for stdin redirector.
 * stdin redirects stdin to serial (write op).
 * Return true if F1 key is pushed (active close).
 */
DWORD WINAPI StdInRedirector(_In_ LPVOID lpParameter) {
	SimpleCom::TStdInRedirectorParam* param = reinterpret_cast<SimpleCom::TStdInRedirectorParam*>(lpParameter);
	INPUT_RECORD inputs[buf_sz];
	DWORD n_read;

	CONSOLE_SCREEN_BUFFER_INFO console_info = { 0 };
	CALL_WINAPI_WITH_DEBUGLOG(GetConsoleScreenBufferInfo(param->hStdOut, &console_info), TRUE, __FILE__, __LINE__)
	COORD current_window_sz = console_info.dwSize;

	SimpleCom::SerialPortWriter writer(param->hSerial, buf_sz);

	try {
		HANDLE waiters[] = { param->hStdIn, param->hTermEvent };
		COORD newConsoleSize;
		bool isConsoleSizeUpdated = false;

		while (true) {
			DWORD result = WaitForMultipleObjects(sizeof(waiters) / sizeof(HANDLE), waiters, FALSE, INFINITE);
			if (result == WAIT_OBJECT_0) { // hStdIn
				if (!ReadConsoleInput(param->hStdIn, inputs, sizeof(inputs) / sizeof(INPUT_RECORD), &n_read)) {
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
							if (ShouldTerminate(param->parent_hwnd, writer, param->hTermEvent)) {
								*param->reattachable = false;
								CALL_WINAPI_WITH_DEBUGLOG(CancelIoEx(param->hSerial, nullptr), TRUE, __FILE__, __LINE__)
								return 0;
							}
							else {
								continue;
							}
						}

						ProcessKeyEvents(inputs[idx].Event.KeyEvent, writer, param->enableStdinLogging ? param->logwriter : nullptr);
					}
					else if ((inputs[idx].EventType == WINDOW_BUFFER_SIZE_EVENT) && param->useTTYResizer) {
						if (memcmp(&current_window_sz, &inputs[idx].Event.WindowBufferSizeEvent.dwSize, sizeof(COORD)) != 0) {
							newConsoleSize = inputs[idx].Event.WindowBufferSizeEvent.dwSize;
							isConsoleSizeUpdated = true;
						}
					}

					if (isConsoleSizeUpdated) {
						DWORD numOfEvents;
						if (GetNumberOfConsoleInputEvents(param->hStdIn, &numOfEvents) && (numOfEvents == 0)) {
							char buf[RINGBUF_SZ];
							int len = snprintf(buf, sizeof(buf), "%c%d" RESIZER_SEPARATOR "%d%c", RESIZER_START_MARKER, newConsoleSize.Y, newConsoleSize.X, RESIZER_END_MARKER);
							writer.PutData(buf, len);
							isConsoleSizeUpdated = false;
							current_window_sz = newConsoleSize;
						}
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
		// Fire terminate event because other threads should be terminated immediately.
		SetEvent(param->hTermEvent);
		param->exception_handler(e);
	}

	writer.Shutdown();
	return 0;
}

SimpleCom::TerminalRedirector::TerminalRedirector(HANDLE hSerial, SimpleCom::LogWriter* logwriter, bool enableStdinLogging, bool useTTYResizer, HWND parent_hwnd) :
	TerminalRedirectorBase(hSerial),
	_hTermEvent(CreateEvent(NULL, TRUE, FALSE, NULL), _T("CreateEvent for thread termination")),
	_hIoEvent(CreateEvent(NULL, TRUE, TRUE, NULL), _T("CreateEvent for reading from serial device")),
	_exception_queue(),
	_reattachable(true)
{
	TStringStream ss;
	ss << "Current code page: " << GetConsoleCP();
	SimpleCom::debug::log(ss.str().c_str());

	DWORD mode;
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdIn == INVALID_HANDLE_VALUE) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetStdHandle(stdin)"));
	}
	CALL_WINAPI_WITH_DEBUGLOG(GetConsoleMode(hStdIn, &mode), TRUE, __FILE__, __LINE__)
		mode &= ~ENABLE_PROCESSED_INPUT;
	mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
	CALL_WINAPI_WITH_DEBUGLOG(SetConsoleMode(hStdIn, mode), TRUE, __FILE__, __LINE__)

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetStdHandle(stdout)"));
	}
	CALL_WINAPI_WITH_DEBUGLOG(GetConsoleMode(hStdOut, &mode), TRUE, __FILE__, __LINE__);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT;
	CALL_WINAPI_WITH_DEBUGLOG(SetConsoleMode(hStdOut, mode), TRUE, __FILE__, __LINE__);

	_stdin_param = {
		.hSerial = hSerial,
		.hStdIn = hStdIn,
		.hStdOut = hStdOut,
		.enableStdinLogging = enableStdinLogging,
		.logwriter = logwriter,
		.useTTYResizer = useTTYResizer,
		.parent_hwnd = parent_hwnd,
		.hTermEvent = _hTermEvent.handle(),
		.exception_handler = [&](const WinAPIException& e) { _exception_queue.push(e); },
		.reattachable = &_reattachable
	};

	_stdout_param = {
		.hSerial = hSerial,
		.hStdOut = hStdOut,
		.overlapped = { .hEvent = _hIoEvent.handle() },
		.logwriter = logwriter,
		.hTermEvent = _hTermEvent.handle(),
		.exception_handler = [&](const WinAPIException& e) { _exception_queue.push(e); }
	};
}

std::tuple<LPTHREAD_START_ROUTINE, LPVOID> SimpleCom::TerminalRedirector::GetStdInRedirector() {
	return { &StdInRedirector, &_stdin_param };
}

std::tuple<LPTHREAD_START_ROUTINE, LPVOID> SimpleCom::TerminalRedirector::GetStdOutRedirector() {
	return { &StdOutRedirector, &_stdout_param };
}

void SimpleCom::TerminalRedirector::StartRedirector(){
	// Clear console
	WriteConsole(_stdout_param.hStdOut, CLEAR_CONSOLE_COMMAND, CLEAR_CONSOLE_COMMAND_LEN, nullptr, nullptr);

	TerminalRedirectorBase::StartRedirector();
}

bool SimpleCom::TerminalRedirector::Reattachable() {
	return _reattachable;
}