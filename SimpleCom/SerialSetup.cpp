/*
 * Copyright (C) 2019, 2023, Yasumasa Suenaga
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


// default option parser for integer types
template<typename T> void SimpleCom::CommandlineOption<T>::set_from_arg(LPCTSTR arg) {
	size_t idx;
	_value = std::stoi(arg, &idx);
	if (arg[idx] != 0) {
		throw std::invalid_argument("Could not convert to integer");
	}
}

void SimpleCom::CommandlineOption<bool>::set_from_arg(LPCTSTR arg) {
	_value = true;
}

void SimpleCom::CommandlineOption<SimpleCom::Parity>::set_from_arg(LPCTSTR arg) {
	if (_tcscmp(_T("none"), arg) == 0) {
		_value = const_cast<Parity&>(NO_PARITY);
	}
	else if (_tcscmp(_T("odd"), arg) == 0) {
		_value = const_cast<Parity&>(ODD_PARITY);
	}
	else if (_tcscmp(_T("even"), arg) == 0) {
		_value = const_cast<Parity&>(EVEN_PARITY);
	}
	else if (_tcscmp(_T("mark"), arg) == 0) {
		_value = const_cast<Parity&>(MARK_PARITY);
	}
	else if (_tcscmp(_T("space"), arg) == 0) {
		_value = const_cast<Parity&>(SPACE_PARITY);
	}
	else {
		throw std::invalid_argument("Parity: unknown argument");
	}
}

void SimpleCom::CommandlineOption<SimpleCom::StopBits>::set_from_arg(LPCTSTR arg) {
	if (_tcscmp(_T("1"), arg) == 0) {
		_value = const_cast<StopBits&>(ONE);
	}
	else if (_tcscmp(_T("1.5"), arg) == 0) {
		_value = const_cast<StopBits&>(ONE5);
	}
	else if (_tcscmp(_T("2"), arg) == 0) {
		_value = const_cast<StopBits&>(TWO);
	}
	else {
		throw std::invalid_argument("StopBits: unknown argument");
	}
}

void SimpleCom::CommandlineOption<SimpleCom::FlowControl>::set_from_arg(LPCTSTR arg) {
	if (_tcscmp(_T("none"), arg) == 0) {
		_value = const_cast<FlowControl&>(NONE);
	}
	else if (_tcscmp(_T("hardware"), arg) == 0) {
		_value = const_cast<FlowControl&>(HARDWARE);
	}
	else if (_tcscmp(_T("software"), arg) == 0) {
		_value = const_cast<FlowControl&>(SOFTWARE);
	}
	else {
		throw std::invalid_argument("FlowControl: unknown argument");
	}
}

#ifdef _UNICODE
#define COUT std::wcout
#else
#define COUT std::cout
#endif

void SimpleCom::CommandlineHelpOption::set_from_arg(LPCTSTR arg) {
	COUT << R"(
Usage:
  SimpleCom.exe <options> <COM port>

Options:)" << std::endl;

	for (auto itr = _options->begin(); itr != _options->end(); itr++) {
		auto opt = itr->second;
		COUT << _T("   ") << itr->first << _T(" ") << opt->GetArgs() << std::endl;
		COUT << _T("   \t") << opt->GetDescription() << std::endl;
	}
	ExitProcess(100);
}

SimpleCom::SerialSetup::SerialSetup() :
	_port(),
	_scanner(),
	_options()
{
	_options[_T("--baud-rate")] = new CommandlineOption<DWORD>(_T("[num]"), _T("Baud rate"), 115200);
	_options[_T("--byte-size")] = new CommandlineOption<BYTE>(_T("[num]"), _T("Byte size"), 8);
	_options[_T("--parity")] = new CommandlineOption<Parity>(_T("[none|odd|even|mark|space]"), _T("Parity"), const_cast<Parity&>(parities[0])); // NO_PARITY
	_options[_T("--stop-bits")] = new CommandlineOption<StopBits>(_T("[1|1.5|2]"), _T("Stop bits"), const_cast<StopBits&>(stopbits[0])); // ONE
	_options[_T("--flow-control")] = new CommandlineOption<FlowControl>(_T("[none|hardware|software]"), _T("Flow control"), const_cast<FlowControl&>(flowctrls[0])); // NONE
	_options[_T("--utf8")] = new CommandlineOption<bool>(_T(""), _T("Use UTF-8 code page"), false);
	_options[_T("--tty-resizer")] = new CommandlineOption<bool>(_T(""), _T("Use TTY Resizer"), false);
	_options[_T("--show-dialog")] = new CommandlineOption<bool>(_T(""), _T("Show setup dialog"), false);
	_options[_T("--wait-serial-device")] = new CommandlineOption<int>(_T("[num]"), _T("Seconds to wait for serial device"), 0);
	_options[_T("--auto-reconnect")] = new CommandlineOption<bool>(_T(""), _T("Reconnect to peripheral automatically"), false);
	_options[_T("--auto-reconnect-pause")] = new CommandlineOption<int>(_T("[num]"), _T("Pause time in seconds before reconnecting"), 3);
	_options[_T("--auto-reconnect-timeout")] = new CommandlineOption<int>(_T("[num]"), _T("Reconnect timeout"), 120);
	_options[_T("--help")] = new CommandlineHelpOption(&_options);
}


SimpleCom::SerialSetup::~SerialSetup()
{
	for (auto itr = _options.begin(); itr != _options.end(); itr++) {
		delete itr->second;
	}
}

static void AddStringToComboBox(HWND hCombo, TString str) {
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

	TString text_str;
	WPARAM cb_idx;

	HWND hComboSerialDevice = GetDlgItem(hDlg, IDC_SERIAL_DEVICE);
	if (hComboSerialDevice == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_SERIAL_DEVICE)"));
	}
	cb_idx = 0;
	auto devices = setup->GetDeviceScanner().GetDevices();
	for (auto itr : devices) {
		const TString port = itr.first;
		text_str = port + _T(": ") + itr.second;
		AddStringToComboBox(hComboSerialDevice, text_str);
		if (port == setup->GetPort()) {
			auto target_itr = devices.find(itr.first);
			cb_idx = std::distance(devices.begin(), target_itr);
		}
	}
	SendMessage(hComboSerialDevice, CB_SETCURSEL, cb_idx, 0);

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
	cb_idx = 0;
	for (auto& parity : parities) {
		AddStringToComboBox(hComboParity, parity.tstr());
		if (parity == setup->GetParity()) {
			cb_idx = &parity - parities;
		}
	}
	SendMessage(hComboParity, CB_SETCURSEL, cb_idx, 0);


	HWND hComboStopBits = GetDlgItem(hDlg, IDC_STOP_BITS);
	if (hComboStopBits == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_STOP_BITS)"));
	}
	cb_idx = 0;
	for (auto& stopbit : stopbits) {
		AddStringToComboBox(hComboStopBits, stopbit.tstr());
		if (stopbit == setup->GetStopBits()) {
			cb_idx = &stopbit - stopbits;
		}
	}
	SendMessage(hComboStopBits, CB_SETCURSEL, cb_idx, 0);

	HWND hComboFlowCtl = GetDlgItem(hDlg, IDC_FLOW_CTL);
	if (hComboFlowCtl == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_FLOW_CTL)"));
	}
	cb_idx = 0;
	for (auto& flowctl : flowctrls) {
		AddStringToComboBox(hComboFlowCtl, flowctl.tstr());
		if (flowctl == setup->GetFlowControl()) {
			cb_idx = &flowctl - flowctrls;
		}
	}
	SendMessage(hComboFlowCtl, CB_SETCURSEL, cb_idx, 0);

	HWND hCheckUTF8 = GetDlgItem(hDlg, IDC_CHECK_UTF8);
	if (hCheckUTF8 == nullptr) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_CHECK_UTF8)"));
	}
	SendMessage(hCheckUTF8, BM_SETCHECK, setup->GetUseUTF8() ? BST_CHECKED : BST_UNCHECKED, 0);

	HWND hCheckTTYResizer = GetDlgItem(hDlg, IDC_CHECK_TTY_RESIZER);
	if (hCheckTTYResizer == nullptr) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_CHECK_TTY_RESIZER)"));
	}
	SendMessage(hCheckTTYResizer, BM_SETCHECK, setup->GetUseTTYResizer() ? BST_CHECKED : BST_UNCHECKED, 0);

	HWND hCheckAutoReconnect = GetDlgItem(hDlg, IDC_CHECK_AUTO_RECONNECT);
	if (hCheckAutoReconnect == nullptr) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_CHECK_AUTO_RECONNECT)"));
	}
	SendMessage(hCheckAutoReconnect, BM_SETCHECK, setup->GetAutoReconnect() ? BST_CHECKED : BST_UNCHECKED, 0);

	text_str = TO_STRING(setup->GetAutoReconnectPauseInSec());
	if (!SetDlgItemText(hDlg, IDC_RECONNECT_PAUSE, text_str.c_str())) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_RECONNECT_PAUSE)"));
	}

	text_str = TO_STRING(setup->GetAutoReconnectTimeoutInSec());
	if (!SetDlgItemText(hDlg, IDC_RECONNECT_TIMEOUT, text_str.c_str())) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_RECONNECT_TIMEOUT)"));
	}

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
	auto target = setup->GetDeviceScanner().GetDevices().begin();
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

	auto checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_UTF8), BM_GETCHECK, 0, 0);
	setup->SetUseUTF8(checked == BST_CHECKED);

	checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_TTY_RESIZER), BM_GETCHECK, 0, 0);
	setup->SetUseTTYResizer(checked == BST_CHECKED);

	checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_AUTO_RECONNECT), BM_GETCHECK, 0, 0);
	setup->SetAutoReconnect(checked == BST_CHECKED);

	intval = GetDlgItemInt(hDlg, IDC_RECONNECT_PAUSE, &translated, FALSE);
	if (!translated) {
		return false;
	}
	setup->SetAutoReconnectPauseInSec(intval);

	intval = GetDlgItemInt(hDlg, IDC_RECONNECT_TIMEOUT, &translated, FALSE);
	if (!translated) {
		return false;
	}
	setup->SetAutoReconnectTimeoutInSec(intval);

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
	for (int i = 1; i < argc; i++) {
		if (_tcsncmp(_T("--"), argv[i], 2) == 0) {
			auto itr = _options.find(argv[i]);
			if (itr != _options.end()) {
				CommandlineOptionBase* opt = itr->second;
				if (opt->need_arguments()) {
					if (++i == argc) {
						throw SerialSetupException(_T("command line argument"), _T("Insufficient command line arguments"));
					}
					try {
						opt->set_from_arg(argv[i]);
					}
					catch (const std::invalid_argument& e) {
						// Only ASCII chars should be converted to wchar, so we can ignore deprecation since C++17.
						// "/D _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" has been added to command line option in project properties.
						std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
						throw SerialSetupException(_T("command line argument"), conv.from_bytes(e.what()).c_str());
					}
				}
				else {
					// This should be bool type, should set to true, and it expects not to throw any exceptions.
					opt->set_from_arg(nullptr);
				}
			}
			else {
				throw SerialSetupException(_T("command line argument (unknown)"), argv[i]);
			}
		}
		else {
			TRegex re(_T("^COM\\d+$"));
			if (std::regex_match(argv[i], re)) {
				SetPort(argv[i]);
			}
			else {
				throw SerialSetupException(_T("command line argument (unknown)"), argv[i]);
			}
		}
	}

	if (!IsShowDialog() && _port.empty()) {
		throw SerialSetupException(_T("command line argument"), _T("Serial port is not specified"));
	}
}

/*
 * Save configuration to DCB
 */
void SimpleCom::SerialSetup::SaveToDCB(LPDCB dcb) noexcept {
	ZeroMemory(dcb, sizeof(DCB));
	dcb->DCBlength = sizeof(DCB);

	dcb->BaudRate = GetBaudRate();
	dcb->fBinary = TRUE;
	dcb->fParity = GetParity() != NO_PARITY;
	dcb->Parity = GetParity();
	dcb->fDtrControl = DTR_CONTROL_ENABLE;
	dcb->fRtsControl = RTS_CONTROL_ENABLE;

	if (GetFlowControl() == HARDWARE) {
		dcb->fOutxCtsFlow = TRUE;
		dcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	else if (GetFlowControl() == SOFTWARE) {
		dcb->fOutX = TRUE;
		dcb->fInX = TRUE;
		dcb->XonLim = 2048;
		dcb->XoffLim = 2048;
		dcb->XonChar = 0x11;
		dcb->XoffChar = 0x13;
	}

	dcb->ByteSize = GetByteSize();
	dcb->StopBits = GetStopBits();
}
