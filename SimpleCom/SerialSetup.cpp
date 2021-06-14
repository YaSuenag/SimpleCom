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
#include "WinAPIException.h"
#include "resource.h"


#ifdef _UNICODE
#define TO_STRING(x) std::to_wstring(x)
#else
#define TO_STRING(x) std::to_string(x)
#endif


// Enum setup for parity
static constexpr SimpleCom::Parity NO_PARITY{ NOPARITY, _T("none") };
static constexpr SimpleCom::Parity ODD_PARITY{ ODDPARITY, _T("odd") };
static constexpr SimpleCom::Parity EVEN_PARITY{ EVENPARITY, _T("even") };
static constexpr SimpleCom::Parity MARK_PARITY{ MARKPARITY, _T("mark") };
static constexpr SimpleCom::Parity SPACE_PARITY{ SPACEPARITY, _T("space") };
static constexpr SimpleCom::Parity parities[] = { NO_PARITY, ODD_PARITY, EVEN_PARITY, MARK_PARITY, SPACE_PARITY };

// Enum setup for flow control
static constexpr SimpleCom::FlowControl NONE{ __COUNTER__, _T("none") };
static constexpr SimpleCom::FlowControl HARDWARE{ __COUNTER__, _T("hardware") };
static constexpr SimpleCom::FlowControl SOFTWARE{ __COUNTER__, _T("software") };
static constexpr SimpleCom::FlowControl flowctrls[] = { NONE, HARDWARE, SOFTWARE };

// Enum setup for stop bits
static constexpr SimpleCom::StopBits ONE{ ONESTOPBIT, _T("1") };
static constexpr SimpleCom::StopBits ONE5{ ONE5STOPBITS, _T("1.5") };
static constexpr SimpleCom::StopBits TWO{ TWOSTOPBITS, _T("2") };
static constexpr SimpleCom::StopBits stopbits[] = { ONE, ONE5, TWO };


SimpleCom::SerialSetup::SerialSetup() : _port(),
										_baud_rate(115200),
										_byte_size(8),
										_parity(const_cast<Parity&>(parities[0])), // NO__PARITY
										_stop_bits(const_cast<StopBits&>(stopbits[0])), // ONE
										_flow_control(const_cast<FlowControl&>(flowctrls[0])), // NONE
										_devices()
{
	initialize();
}


SimpleCom::SerialSetup::~SerialSetup()
{
	// Do nothing
}

/*
 * RAII class for Registry key
 */
class RegistryKeyHandler {
private:
	HKEY hKey;

public:
	RegistryKeyHandler(HKEY hOpenKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired) {
		LSTATUS status = RegOpenKeyEx(hOpenKey, lpSubKey, ulOptions, samDesired, &hKey);
		if (status != ERROR_SUCCESS) {
			hKey = static_cast<HKEY>(INVALID_HANDLE_VALUE);
			throw SimpleCom::WinAPIException(GetLastError(), lpSubKey);
		}
	}

	~RegistryKeyHandler() {
		if (hKey != INVALID_HANDLE_VALUE) {
			RegCloseKey(hKey);
		}
	}

	HKEY key() {
		return hKey;
	}
};

void SimpleCom::SerialSetup::initialize() {
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
		throw SerialSetupException(_T("configuration"), _T("Serial interface not found"));
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

static void AddStringToComboBox(HWND hCombo, SimpleCom::TString str) {
	LRESULT result = SendMessage(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
	if (result == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("CB_ERR"), _T("AddStringToComboBox"));
	}
	else if (result == CB_ERRSPACE) {
		throw SimpleCom::WinAPIException(_T("CB_ERRSPACE"), _T("AddStringToComboBox"));
	}
}

static void InitializeDialog(HWND hDlg, SimpleCom::SerialSetup *setup) {
	// Initialize serial configuration.
	// Initial value is for serial console of Raspberry Pi.

	SimpleCom::TString text_str;

	HWND hComboSerialDevice = GetDlgItem(hDlg, IDC_SERIAL_DEVICE);
	if (hComboSerialDevice == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_SERIAL_DEVICE)"));
	}
	for (auto itr = setup->GetDevices().begin(); itr != setup->GetDevices().end(); itr++) {
		text_str = itr->first + _T(": ") + itr->second;
		AddStringToComboBox(hComboSerialDevice, text_str);
	}
	SendMessage(hComboSerialDevice, CB_SETCURSEL, 0, 0);

	text_str = TO_STRING(setup->GetBaudRate());
	if (!SetDlgItemText(hDlg, IDC_BAUD_RATE, text_str.c_str())) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_BAUD_RATE)"));
	}

	text_str = TO_STRING(setup->GetByteSize());
	if (!SetDlgItemText(hDlg, IDC_BYTE_SIZE, text_str.c_str())) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_BYTE_SIZE)"));
	}

	HWND hComboParity = GetDlgItem(hDlg, IDC_PARITY);
	if (hComboParity == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_PARITY)"));
	}
	for (auto parity : parities) {
		AddStringToComboBox(hComboParity, parity.tstr());
	}
	SendMessage(hComboParity, CB_SETCURSEL, 0, 0);


	HWND hComboStopBits = GetDlgItem(hDlg, IDC_STOP_BITS);
	if (hComboStopBits == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_STOP_BITS)"));
	}
	for (auto stopbit : stopbits) {
		AddStringToComboBox(hComboStopBits, stopbit.tstr());
	}
	SendMessage(hComboStopBits, CB_SETCURSEL, 0, 0);

	HWND hComboFlowCtl = GetDlgItem(hDlg, IDC_FLOW_CTL);
	if (hComboFlowCtl == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_FLOW_CTL)"));
	}
	for (auto flowctl : flowctrls) {
		AddStringToComboBox(hComboFlowCtl, flowctl.tstr());
	}
	SendMessage(hComboFlowCtl, CB_SETCURSEL, 0, 0);
}

/*
 * Retrieves serial configuration from dialog.
 * Return true if all configuration are retrieved and they are valid.
 */
static bool GetConfigurationFromDialog(HWND hDlg, SimpleCom::SerialSetup* setup) {
	int selected_idx;

	// Iterating order of std::map is guaranteed.
	// So we can get map entry from the index of combo box.
	// https://stackoverflow.com/questions/7648756/is-the-order-of-iterating-through-stdmap-known-and-guaranteed-by-the-standard
	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_SERIAL_DEVICE), CB_GETCURSEL, 0, 0));
	if (selected_idx == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("No item is selected"), _T("IDC_SERIAL_DEVICE"));
	}
	auto target = setup->GetDevices().begin();
	for (int i = 0; i < selected_idx; i++, target++) {
		// Do nothing
	}
	setup->SetPort(target->first);

	int intval;
	BOOL translated;

	intval = GetDlgItemInt(hDlg, IDC_BAUD_RATE, &translated, FALSE);
	if (!translated) {
		return false;
	}
	setup->SetBaudRate(intval);

	intval = GetDlgItemInt(hDlg, IDC_BYTE_SIZE, &translated, FALSE);
	if (!translated) {
		return false;
	}
	setup->SetByteSize(intval);

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_PARITY), CB_GETCURSEL, 0, 0));
	if (selected_idx == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("No item is selected"), _T("IDC_PARITY"));
	}
	setup->SetParity(const_cast<SimpleCom::Parity &>(parities[selected_idx]));

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_STOP_BITS), CB_GETCURSEL, 0, 0));
	if (selected_idx == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("No item is selected"), _T("IDC_STOP_BITS"));
	}
	setup->SetStopBits(const_cast<SimpleCom::StopBits&>(stopbits[selected_idx]));

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_FLOW_CTL), CB_GETCURSEL, 0, 0));
	if (selected_idx == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("No item is selected"), _T("IDC_FLOW_CTL"));
	}
	setup->SetFlowControl(const_cast<SimpleCom::FlowControl&>(flowctrls[selected_idx]));

	return true;
}

/*
 * Configuration dialog box procedure
 */
static INT_PTR CALLBACK SettingDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	static SimpleCom::SerialSetup* setup;
	
	try {
		switch (msg) {
		case WM_INITDIALOG:
			setup = reinterpret_cast<SimpleCom::SerialSetup*>(lParam);
			InitializeDialog(hDlg, setup);
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDCONNECT) {
				GetConfigurationFromDialog(hDlg, setup);
				EndDialog(hDlg, IDCONNECT);
				return TRUE;
			}
			else if (LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			}
			return FALSE;

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
 * Show serial configuration dialog.
 * Return true if the dialog is finished with "connect" button (IDCONNECT).
 */
bool SimpleCom::SerialSetup::ShowConfigureDialog(HINSTANCE hInst, HWND hWnd) noexcept {
	return IDCONNECT == DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SETUP), hWnd, &SettingDlgProc, reinterpret_cast<LPARAM>(this));
}

/**
 * Parse command line arguments, and set them to this instance.
 */
void SimpleCom::SerialSetup::ParseArguments(int argc, LPCTSTR argv[]) {
	int i = 1;
	for (; i < argc; i++) {
		if ((_tcsncmp(_T("--"), argv[i], 2) == 0) && ((i + 1) == argc)) {
			throw SerialSetupException(_T("command line argument"), _T("Insufficient command line arguments"));
		}

		if (_tcscmp(_T("--baud-rate"), argv[i]) == 0) {
			try {
				size_t idx;
				LPCTSTR str = argv[++i];
				_baud_rate = std::stoi(str, &idx);
				if (str[idx] != 0) {
					throw std::invalid_argument("--baud-rate");
				}
			}
			catch (...) {
				throw SerialSetupException(_T("command line argument"), _T("Invalid argument: --baud-rate"));
			}
		}
		else if (_tcscmp(_T("--byte-size"), argv[i]) == 0) {
			try {
				size_t idx;
				LPCTSTR str = argv[++i];
				_byte_size = std::stoi(str, &idx);
				if (str[idx] != 0) {
					throw std::invalid_argument("--byte-size");
				}
			}
			catch (...) {
				throw SerialSetupException(_T("command line argument"), _T("Invalid argument: --byte-size"));
			}
		}
		else if (_tcscmp(_T("--parity"), argv[i]) == 0) {
			LPCTSTR parity_val = argv[++i];
			if (_tcscmp(_T("none"), parity_val) == 0) {
				_parity = const_cast<Parity&>(NO_PARITY);
			}
			else if (_tcscmp(_T("odd"), parity_val) == 0) {
				_parity = const_cast<Parity&>(ODD_PARITY);
			}
			else if (_tcscmp(_T("even"), parity_val) == 0) {
				_parity = const_cast<Parity&>(EVEN_PARITY);
			}
			else if (_tcscmp(_T("mark"), parity_val) == 0) {
				_parity = const_cast<Parity&>(MARK_PARITY);
			}
			else if (_tcscmp(_T("space"), parity_val) == 0) {
				_parity = const_cast<Parity&>(SPACE_PARITY);
			}
			else {
				throw SerialSetupException(_T("command line argument"), _T("Invalid argument: --parity"));
			}
		}
		else if (_tcscmp(_T("--stop-bits"), argv[i]) == 0) {
			LPCTSTR sb_val = argv[++i];
			if (_tcscmp(_T("1"), sb_val) == 0) {
				_stop_bits = const_cast<StopBits&>(ONE);
			}
			else if (_tcscmp(_T("1.5"), sb_val) == 0) {
				_stop_bits = const_cast<StopBits&>(ONE5);
			}
			else if (_tcscmp(_T("2"), sb_val) == 0) {
				_stop_bits = const_cast<StopBits&>(TWO);
			}
			else {
				throw SerialSetupException(_T("command line argument"), _T("Invalid argument: --stop-bits"));
			}
		}
		else if (_tcscmp(_T("--flow-control"), argv[i]) == 0) {
			LPCTSTR fc_val = argv[++i];
			if (_tcscmp(_T("none"), fc_val) == 0) {
				_flow_control = const_cast<FlowControl&>(NONE);
			}
			else if (_tcscmp(_T("hardware"), fc_val) == 0) {
				_flow_control = const_cast<FlowControl&>(HARDWARE);
			}
			else if (_tcscmp(_T("software"), fc_val) == 0) {
				_flow_control = const_cast<FlowControl&>(SOFTWARE);
			}
			else {
				throw SerialSetupException(_T("command line argument"), _T("Invalid argument: --flow-control"));
			}
		}
		else {
#ifdef _UNICODE
			std::wregex re(_T("^COM\\d+$"));
#else
			std::regex re("^COM\\d+$");
#endif
			if (std::regex_match(argv[i], re)) {
				SetPort(argv[i]);
			}
			else {
				throw SerialSetupException(_T("command line argument"), _T("Unknown serial port"));
			}
		}
	}

	if (i != argc) {
		throw SerialSetupException(_T("command line argument"), _T("Illegal command line options"));
	}
	else if (_port.empty()) {
		throw SerialSetupException(_T("command line argument"), _T("Serial port is not specified"));
	}
}

/*
 * Save configuration to DCB
 */
void SimpleCom::SerialSetup::SaveToDCB(LPDCB dcb) noexcept {
	ZeroMemory(dcb, sizeof(DCB));
	dcb->DCBlength = sizeof(DCB);

	dcb->BaudRate = _baud_rate;
	dcb->fBinary = TRUE;
	dcb->fParity = _parity != NO_PARITY;
	dcb->Parity = _parity;
	dcb->fDtrControl = DTR_CONTROL_ENABLE;
	dcb->fRtsControl = RTS_CONTROL_ENABLE;

	if (_flow_control == HARDWARE) {
		dcb->fOutxCtsFlow = TRUE;
		dcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	else if (_flow_control == SOFTWARE) {
		dcb->fOutX = TRUE;
		dcb->fInX = TRUE;
		dcb->XonLim = 2048;
		dcb->XoffLim = 2048;
		dcb->XonChar = 0x11;
		dcb->XoffChar = 0x13;
	}

	dcb->ByteSize = _byte_size;
	dcb->StopBits = _stop_bits;
}
