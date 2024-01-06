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

#include "SerialSetup.h"
#include "SerialDeviceScanner.h"
#include "SerialConnection.h"
#include "WinAPIException.h"
#include "debug.h"


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

static std::tuple<HANDLE, HANDLE> InitConsole(SimpleCom::SerialSetup& setup) {
	TStringStream ss;
	ss << "Current code page: " << GetConsoleCP();
	SimpleCom::debug::log(ss.str().c_str());

	if (setup.GetUseUTF8()) {
		CALL_WINAPI_WITH_DEBUGLOG(SetConsoleCP(CP_UTF8), TRUE, __FILE__, __LINE__);
		CALL_WINAPI_WITH_DEBUGLOG(SetConsoleOutputCP(CP_UTF8), TRUE, __FILE__, __LINE__);
		TStringStream ss2;
		ss2 << "Code page changed: " << GetConsoleCP();
		SimpleCom::debug::log(ss2.str().c_str());
	}

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

	return { hStdIn, hStdOut };
}

int _tmain(int argc, LPCTSTR argv[])
{
	DCB dcb;
	TString device;
	std::tuple<HANDLE, HANDLE> std_handles;
	HWND parent_hwnd = GetParentWindow();
	SimpleCom::SerialSetup setup;

	// Serial port configuration
	try {
		if (argc > 1) {
			// command line mode
			setup.ParseArguments(argc, argv);
		}
		else {
			// GUI mode
			setup.SetShowDialog(true);
		}

		std_handles = InitConsole(setup);

		if (setup.GetWaitDevicePeriod() > 0) {
			setup.GetDeviceScanner().SetTargetPort(setup.GetPort());
			setup.GetDeviceScanner().WaitSerialDevices(parent_hwnd, setup.GetWaitDevicePeriod());
			if (setup.GetDeviceScanner().GetDevices().empty()) {
				throw SimpleCom::SerialDeviceScanException(_T("Waiting for serial device"), _T("Serial device is not available"));
			}
		}
		else {
			setup.GetDeviceScanner().ScanSerialDevices();
		}

		if (setup.IsShowDialog() && !setup.ShowConfigureDialog(nullptr, parent_hwnd)) {
			return -1;
		}

		device = _T(R"(\\.\)") + setup.GetPort();
		setup.SaveToDCB(&dcb);
	}
	catch (SimpleCom::WinAPIException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -1;
	}
	catch (SimpleCom::SerialSetupException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -2;
	}
	catch (SimpleCom::SerialDeviceScanException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -3;
	}

	try {
		while (true) {
			SimpleCom::SerialConnection conn(device, &dcb, parent_hwnd, std::get<0>(std_handles), std::get<1>(std_handles), setup.GetUseTTYResizer(), setup.GetLogFile(), setup.IsEnableStdinLogging());
			bool exited = conn.DoSession();

			if (setup.GetAutoReconnect() && !exited) {
				SimpleCom::debug::log(_T("Sleep before reconnecting..."));
				Sleep(setup.GetAutoReconnectPauseInSec() * 1000);

				SimpleCom::debug::log(_T("Reconnect start"));
				SimpleCom::SerialDeviceScanner scanner;
				scanner.SetTargetPort(setup.GetPort());
				RegDisablePredefinedCacheEx();
				scanner.WaitSerialDevices(parent_hwnd, setup.GetAutoReconnectTimeoutInSec());
				if (scanner.GetDevices().empty()) {
					throw SimpleCom::SerialDeviceScanException(_T("Waiting for serial device"), _T("Serial device is not available"));
				}
				SimpleCom::debug::log(_T("Reconnect device found"));
			}
			else {
				break;
			}
		}
	}
	catch (SimpleCom::WinAPIException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -4;
	}
	catch (SimpleCom::SerialDeviceScanException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -5;
	}

	return 0;
}
