/*
 * Copyright (C) 2019, 2025, Yasumasa Suenaga
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

static int DoInteractiveMode(TString& device, DCB *dcb, SimpleCom::SerialSetup &setup, HWND parent_hwnd) {
	try {
		while (true) {
			SimpleCom::SerialConnection conn(device, dcb, setup.GetLogFile(), setup.IsEnableStdinLogging());
			bool reattachable = conn.DoSession(setup.GetAutoReconnect(), setup.GetUseTTYResizer(), parent_hwnd);

			if (setup.GetAutoReconnect() && reattachable) {
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
		MessageBox(parent_hwnd, e.GetErrorText().c_str(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -4;
	}
	catch (SimpleCom::SerialDeviceScanException& e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -5;
	}

	return 0;
}

static int DoBatchMode(TString& device, DCB* dcb) {
	SimpleCom::SerialConnection conn(device, dcb);
	conn.DoBatch();

	return 0;
}

// https://devblogs.microsoft.com/performance-diagnostics/reduce-process-interference-with-task-manager-efficiency-mode/
static void SetEfficiencyMode() {
	HANDLE hProc = GetCurrentProcess();

	// a) reduces process base priority to low
	SetPriorityClass(hProc, IDLE_PRIORITY_CLASS);

	// b) sets QoS mode to EcoQoS.
	PROCESS_POWER_THROTTLING_STATE PowerThrottling = {
		.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION,
		.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED,
		.StateMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED
	};
	CALL_WINAPI_WITH_DEBUGLOG(SetProcessInformation(hProc, ProcessPowerThrottling, &PowerThrottling, sizeof(PowerThrottling)), TRUE, __FILE__, __LINE__);
}

int _tmain(int argc, LPCTSTR argv[])
{
	DCB dcb;
	TString device;
	HWND parent_hwnd = GetParentWindow();
	SimpleCom::SerialSetup setup;

	try {
		// Serial port configuration
		if (argc > 1) {
			// command line mode
			setup.ParseArguments(argc, argv);
		}
		else {
			// GUI mode
			setup.SetShowDialog(true);
		}

		if (setup.IsEfficiencyMode()) {
			SetEfficiencyMode();
		}

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
		MessageBox(parent_hwnd, e.GetErrorText().c_str(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
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
	catch(std::invalid_argument& e) {
		// Only ASCII chars should be converted to wchar, so we can ignore deprecation since C++17.
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
		MessageBox(parent_hwnd, conv.from_bytes(e.what()).c_str(), _T("Invalid argument"), MB_OK | MB_ICONERROR);
		return -4;
	}

	if (setup.GetUseUTF8()) {
		CALL_WINAPI_WITH_DEBUGLOG(SetConsoleCP(CP_UTF8), TRUE, __FILE__, __LINE__);
		CALL_WINAPI_WITH_DEBUGLOG(SetConsoleOutputCP(CP_UTF8), TRUE, __FILE__, __LINE__);
		TStringStream ss2;
		ss2 << "Code page changed: " << GetConsoleCP();
		SimpleCom::debug::log(ss2.str().c_str());
	}

	return setup.IsBatchMode() ? DoBatchMode(device, &dcb) : DoInteractiveMode(device, &dcb, setup, parent_hwnd);
}
