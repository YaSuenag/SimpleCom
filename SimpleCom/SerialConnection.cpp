/*
 * Copyright (C) 2023, 2025, Yasumasa Suenaga
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
#include "TerminalRedirector.h"
#include "BatchRedirector.h"
#include "debug.h"
#include "../common/common.h"

static constexpr int buf_sz = 256;



SimpleCom::SerialConnection::SerialConnection(TString& device, DCB* dcb, LPCTSTR logfilename, bool enableStdinLogging) :
	_device(device),
	_enableStdinLogging(enableStdinLogging)
{
	CopyMemory(&_dcb, dcb, sizeof(_dcb));
	_logwriter = (logfilename == nullptr) ? nullptr : new LogWriter(logfilename);
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
 * Talk with peripheral.
 * Set true to allowDetachDevice if the function would be finished silently when serial controller is detached.
 */
bool SimpleCom::SerialConnection::DoSession(bool allowDetachDevice, bool useTTYResizer, HWND parent_hwnd) {
	HandleHandler hSerial(CreateFile(_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL), _T("Open serial port"));
	InitSerialPort(hSerial.handle());

	TerminalRedirector redirector(hSerial.handle(), _logwriter, _enableStdinLogging, useTTYResizer, parent_hwnd);
	redirector.StartRedirector();

	redirector.AwaitTermination();

	WinAPIException ex;
	while (redirector.exception_queue().try_pop(ex)) {
		switch (ex.GetErrorCode()) {
		// Intended - this error would be reported when the user finishes the session.
		case ERROR_OPERATION_ABORTED:
			continue;

		// Following error codes would be reported when serial controller is detached.
		// https://github.com/microsoft/referencesource/blob/51cf7850defa8a17d815b4700b67116e3fa283c2/System/sys/system/IO/ports/SerialStream.cs#L1739-L1748
		case ERROR_ACCESS_DENIED:
		case ERROR_BAD_COMMAND:
		case ERROR_DEVICE_REMOVED:
			if (ex.IsSerialAPIException() && allowDetachDevice) {
				debug::log(_T("SerialAPIException occurred, but error dialog was suppressed because allowDetachDevice is false."));
				debug::log(ex.GetErrorText().c_str());
				continue;
			}
			[[fallthrough]];

		default:
			MessageBox(parent_hwnd, ex.GetErrorText().c_str(), ex.GetErrorCaption(), MB_OK | MB_ICONERROR);
		}
	}

	return redirector.Reattachable();
}

void SimpleCom::SerialConnection::DoBatch() {
	HandleHandler hSerial(CreateFile(_device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL), _T("Open serial port"));
	InitSerialPort(hSerial.handle());

	BatchRedirector redirector(hSerial.handle());

	redirector.StartRedirector();
	redirector.AwaitTermination();
}