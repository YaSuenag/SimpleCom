/*
 * Copyright (C) 2023, Yasumasa Suenaga
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
#include "SerialDeviceScanner.h"
#include "WinAPIException.h"
#include "util.h"
#include "resource.h"

SimpleCom::SerialDeviceScanner::SerialDeviceScanner(HWND parent_hwnd) : _parent_hwnd(parent_hwnd),
                                                                        _dialog_hwnd(nullptr),
                                                                        _devices(),
                                                                        _target_port()
{
	_device_scan_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (_device_scan_event == INVALID_HANDLE_VALUE) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("CreateEvent for reading from waiting serial device"));
	}
}

SimpleCom::SerialDeviceScanner::~SerialDeviceScanner() {
	CloseHandle(_device_scan_event);
}

/*
 * Waiting device dialog box procedure
 */
static INT_PTR CALLBACK WaitDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	try {
		switch (msg) {
		case WM_INITDIALOG: {
			SendMessage(GetDlgItem(hDlg, IDC_PROGRESS1), PBM_SETMARQUEE, TRUE, 0);
			SimpleCom::SerialDeviceScanner* scanner = reinterpret_cast<SimpleCom::SerialDeviceScanner*>(lParam);
			scanner->SetDialogHwnd(hDlg);
			SetEvent(scanner->GetDeviceScanEvent());
			return TRUE;
		}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			}
			return FALSE;

		case WM_APP:
			EndDialog(hDlg, IDOK);
			return TRUE;

		case WM_CLOSE:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}
	catch (SimpleCom::WinAPIException& e) {
		MessageBox(hDlg, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	return FALSE;
}

/*
 * Entry point for waiting serial device dialog.
 */
DWORD WINAPI WaitSerialDeviceDlgEntry(_In_ LPVOID lpParameter) {
	SimpleCom::SerialDeviceScanner* scanner = reinterpret_cast<SimpleCom::SerialDeviceScanner*>(lpParameter);
	DialogBoxParam(nullptr, MAKEINTRESOURCE(IDD_WAIT_DEVICE), scanner->GetParentHwnd(), &WaitDlgProc, reinterpret_cast<LPARAM>(scanner));
	SetEvent(scanner->GetDeviceScanEvent());
	return 0;
}

/*
 * Entry point for waiting serial device is ready.
 */
DWORD WINAPI WaitSerialDeviceEntry(_In_ LPVOID lpParameter) {
	SimpleCom::SerialDeviceScanner* scanner = reinterpret_cast<SimpleCom::SerialDeviceScanner*>(lpParameter);
	DWORD wait_result;
	while (true) {
		wait_result = WaitForSingleObject(scanner->GetDeviceScanEvent(), 0);
		if (wait_result == WAIT_OBJECT_0) {
			break;
		}

		try {
			scanner->ScanSerialDevices();
			if (!scanner->GetDevices().empty()) {
				auto target = scanner->GetTargetPort();
				bool can_break = target.empty() ? true : scanner->GetDevices().contains(target);
				if (can_break) {
					SendMessage(scanner->GetDialogHwnd(), WM_APP, 0, 0);
					SetEvent(scanner->GetDeviceScanEvent());
					break;
				}
			}
		}
		catch (...) {
			// Ignore
		}
		Sleep(1000);
	}
	return 0;
}

void SimpleCom::SerialDeviceScanner::WaitSerialDevices(const int period) {
	ResetEvent(_device_scan_event); // Use event for preparing dialog
	HandleHandler dialogThreadHnd(CreateThread(nullptr, 0, &WaitSerialDeviceDlgEntry, this, 0, nullptr), _T("CreateThread for WaitSerialDeviceDlgEntry failed"));
	WaitForSingleObject(_device_scan_event, INFINITE); // Dialog is ready

	ResetEvent(_device_scan_event); // Use event for waiting serial device
	HandleHandler scanThreadHnd(CreateThread(nullptr, 0, &WaitSerialDeviceEntry, this, 0, nullptr), _T("CreateThread for WaitSerialDeviceEntry failed"));

	std::exception_ptr e;
	try {
		DWORD result = WaitForSingleObject(_device_scan_event, period * 1000);
		if (result == WAIT_TIMEOUT) {
			SetEvent(_device_scan_event);
			SendMessage(_dialog_hwnd, WM_APP, 0, 0);
			WaitForSingleObject(dialogThreadHnd.handle(), INFINITE);
			WaitForSingleObject(scanThreadHnd.handle(), INFINITE);
			throw SimpleCom::SerialDeviceScanException(_T("Waiting for serial device"), _T("Timed out"));
		}
		else if (result != WAIT_OBJECT_0) {
			WaitForSingleObject(dialogThreadHnd.handle(), INFINITE);
			WaitForSingleObject(scanThreadHnd.handle(), INFINITE);
			throw SimpleCom::WinAPIException(GetLastError(), _T("WaitForSingleObject() for waiting for serial device"));
		}
	}
	catch (...) {
		e = std::current_exception();
	}

	WaitForSingleObject(scanThreadHnd.handle(), INFINITE);
	WaitForSingleObject(dialogThreadHnd.handle(), INFINITE);

	if (e) {
		std::rethrow_exception(e);
	}
}

void SimpleCom::SerialDeviceScanner::ScanSerialDevices() {
	_devices.clear();

	// Generates device map of serial interface name and device name
	// from HKLM\HARDWARE\DEVICEMAP\SERIALCOMM.
	RegistryKeyHandler hkeyHandler(HKEY_LOCAL_MACHINE, _T(R"(HARDWARE\DEVICEMAP\SERIALCOMM)"), REG_OPTION_OPEN_LINK, KEY_READ);
	HKEY hKey = hkeyHandler.key();

	DWORD numValues, maxValueNameLen, maxValueLen;
	LSTATUS status = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &numValues, &maxValueNameLen, &maxValueLen, NULL, NULL);
	if (status != ERROR_SUCCESS) {
		throw WinAPIException(status, _T("RegQueryInfoKey"));
	}
	else if (numValues <= 0) {
		throw SerialDeviceScanException(_T("configuration"), _T("Serial interface not found"));
	}

	maxValueLen++;
	maxValueNameLen++;
	LPTSTR DeviceName = new TCHAR[maxValueLen];
	LPTSTR InterfaceName = new TCHAR[maxValueNameLen];

	for (DWORD idx = 0; idx < numValues; idx++) {
		DWORD ValueNameLen = maxValueNameLen;
		DWORD ValueLen = maxValueLen;

		status = RegEnumValue(hKey, idx, InterfaceName, &ValueNameLen, NULL, NULL, reinterpret_cast<LPBYTE>(DeviceName), &ValueLen);
		if (status == ERROR_SUCCESS) {
			_devices[DeviceName] = InterfaceName;
		}

	}

	delete[] InterfaceName;
	delete[] DeviceName;
}
