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
#include "util.h"
#include "WinAPIException.h"
#include "resource.h"


#ifdef _UNICODE
#define TO_STRING(x) std::to_wstring(x)
#else
#define TO_STRING(x) std::to_string(x)
#endif


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
	for (auto& value : Parity::values) {
		if (_tcscmp(value.tstr(), arg) == 0) {
			_value = value;
			return;
		}
	}
	throw std::invalid_argument("Parity: unknown argument");
}

void SimpleCom::CommandlineOption<SimpleCom::StopBits>::set_from_arg(LPCTSTR arg) {
	for (auto& value : StopBits::values) {
		if (_tcscmp(value.tstr(), arg) == 0) {
			_value = value;
			return;
		}
	}
	throw std::invalid_argument("StopBits: unknown argument");
}

void SimpleCom::CommandlineOption<SimpleCom::FlowControl>::set_from_arg(LPCTSTR arg) {
	for (auto& value : FlowControl::values) {
		if (_tcscmp(value.tstr(), arg) == 0) {
			_value = value;
			return;
		}
	}
	throw std::invalid_argument("FlowControl: unknown argument");
}

void SimpleCom::CommandlineOption<LPTSTR>::set_from_arg(LPCTSTR arg) {
	set(const_cast<LPTSTR>(arg));
}

// constructor
SimpleCom::CommandlineOption<LPTSTR>::CommandlineOption(const TString args, LPCTSTR description, LPTSTR default_val) : CommandlineOptionBase(args, description) {
	_value = (default_val == nullptr) ? nullptr : _tcsdup(default_val);
}

template<typename T> SimpleCom::CommandlineOption<T>::CommandlineOption(const TString args, LPCTSTR description, T default_val) : CommandlineOptionBase(args, description), _value(default_val) {
	// Do nothing
}

// destructor
template<> SimpleCom::CommandlineOption<LPTSTR>::~CommandlineOption() {
	free(_value);
}

template<typename T> SimpleCom::CommandlineOption<T>::~CommandlineOption() {
	// Do nothing
}

// setter
template<> void SimpleCom::CommandlineOption<LPTSTR>::set(LPTSTR new_value) {
	free(nullptr);
	_value = (new_value == nullptr) ? nullptr : _tcsdup(new_value);
}

template<typename T> void SimpleCom::CommandlineOption<T>::set(T new_value) {
	_value = new_value;
}

#ifdef _UNICODE
#define COUT std::wcout
#else
#define COUT std::cout
#endif

void SimpleCom::CommandlineHelpOption::set_from_arg(LPCTSTR arg) {
	ProductInfo prodinfo;

	COUT << prodinfo.GetProductName() << _T(" ") << prodinfo.GetProductVersion() << std::endl;
	COUT << prodinfo.GetLegalCopyright() << std::endl;
	COUT << _T("Usage:") << std::endl;
	COUT << _T("  ") << PathFindFileName(prodinfo.GetExePath()) << _T(" <options> <COM port>") << std::endl;
	COUT << std::endl;
	COUT << _T("Options:") << std::endl;

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
	_options[_T("--parity")] = new CommandlineOption<Parity>(Parity::valueopts(), _T("Parity"), Parity::NO_PARITY);
	_options[_T("--stop-bits")] = new CommandlineOption<StopBits>(StopBits::valueopts(), _T("Stop bits"), StopBits::ONE);
	_options[_T("--flow-control")] = new CommandlineOption<FlowControl>(FlowControl::valueopts(), _T("Flow control"), FlowControl::NONE);
	_options[_T("--utf8")] = new CommandlineOption<bool>(_T(""), _T("Use UTF-8 code page"), false);
	_options[_T("--tty-resizer")] = new CommandlineOption<bool>(_T(""), _T("Use TTY Resizer"), false);
	_options[_T("--show-dialog")] = new CommandlineOption<bool>(_T(""), _T("Show setup dialog"), false);
	_options[_T("--wait-serial-device")] = new CommandlineOption<int>(_T("[num]"), _T("Seconds to wait for serial device"), 0);
	_options[_T("--auto-reconnect")] = new CommandlineOption<bool>(_T(""), _T("Reconnect to peripheral automatically"), false);
	_options[_T("--auto-reconnect-pause")] = new CommandlineOption<int>(_T("[num]"), _T("Pause time in seconds before reconnecting"), 3);
	_options[_T("--auto-reconnect-timeout")] = new CommandlineOption<int>(_T("[num]"), _T("Reconnect timeout"), 120);
	_options[_T("--log-file")] = new CommandlineOption<LPTSTR>(_T("[logfile]"), _T("Log serial communication to file"), nullptr);
	_options[_T("--stdin-logging")] = new CommandlineOption<bool>(_T(""), _T("Enable stdin logging"), false);
	_options[_T("--batch")] = new CommandlineOption<bool>(_T(""), _T("Perform in batch mode"), false);
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

	TStringStream dlg_caption;
	dlg_caption << _T("Serial Connection Setup");
	ProductInfo prodinfo;
	LPCTSTR product_name = prodinfo.GetProductName();
	if (product_name != nullptr) {
		dlg_caption << _T("  -  ") << product_name << _T(" ") << prodinfo.GetProductVersion();
	}
	SetWindowText(hDlg, dlg_caption.str().c_str());

	TString text_str;
	WPARAM cb_idx;

	HWND hComboSerialDevice = GetDlgItem(hDlg, IDC_SERIAL_DEVICE);
	if (hComboSerialDevice == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_SERIAL_DEVICE)"));
	}
	cb_idx = 0;
	auto& devices = setup->GetDeviceScanner().GetDevices();
	for (auto& itr : devices) {
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
	for (int idx = 0; idx < SimpleCom::Parity::values.size(); idx++) {
		SimpleCom::Parity parity = SimpleCom::Parity::values[idx];
		AddStringToComboBox(hComboParity, parity.tstr());
		if (parity == setup->GetParity()) {
			cb_idx = idx;
		}
	}
	SendMessage(hComboParity, CB_SETCURSEL, cb_idx, 0);


	HWND hComboStopBits = GetDlgItem(hDlg, IDC_STOP_BITS);
	if (hComboStopBits == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_STOP_BITS)"));
	}
	cb_idx = 0;
	for (int idx = 0; idx < SimpleCom::StopBits::values.size(); idx++) {
		SimpleCom::StopBits stopbit = SimpleCom::StopBits::values[idx];
		AddStringToComboBox(hComboStopBits, stopbit.tstr());
		if (stopbit == setup->GetStopBits()) {
			cb_idx = idx;
		}
	}
	SendMessage(hComboStopBits, CB_SETCURSEL, cb_idx, 0);

	HWND hComboFlowCtl = GetDlgItem(hDlg, IDC_FLOW_CTL);
	if (hComboFlowCtl == NULL) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_FLOW_CTL)"));
	}
	cb_idx = 0;
	for (int idx = 0; idx < SimpleCom::FlowControl::values.size(); idx++) {
		SimpleCom::FlowControl flowctl = SimpleCom::FlowControl::values[idx];
		AddStringToComboBox(hComboFlowCtl, flowctl.tstr());
		if (flowctl == setup->GetFlowControl()) {
			cb_idx = idx;
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
	EnableWindow(GetDlgItem(hDlg, IDC_RECONNECT_PAUSE), setup->GetAutoReconnect());
	EnableWindow(GetDlgItem(hDlg, IDC_RECONNECT_TIMEOUT), setup->GetAutoReconnect());

	text_str = TO_STRING(setup->GetAutoReconnectPauseInSec());
	if (!SetDlgItemText(hDlg, IDC_RECONNECT_PAUSE, text_str.c_str())) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_RECONNECT_PAUSE)"));
	}

	text_str = TO_STRING(setup->GetAutoReconnectTimeoutInSec());
	if (!SetDlgItemText(hDlg, IDC_RECONNECT_TIMEOUT, text_str.c_str())) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_RECONNECT_TIMEOUT)"));
	}

	HWND hCheckLog = GetDlgItem(hDlg, IDC_CHECK_LOG);
	if (hCheckLog == nullptr) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_CHECK_LOG)"));
	}
	if (setup->GetLogFile() == nullptr) {
		SendMessage(hCheckLog, BM_SETCHECK, BST_UNCHECKED, 0);
	}
	else {
		if (!SetDlgItemText(hDlg, IDC_LOGFILEEDIT, setup->GetLogFile())) {
			throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_LOGFILEEDIT)"));
		}
		SendMessage(hCheckLog, BM_SETCHECK, BST_CHECKED, 0);
		EnableWindow(GetDlgItem(hDlg, IDC_LOGFILEBTN), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_LOGFILEEDIT), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_STDIN_LOGGING), TRUE);
	}

	HWND hCheckStdinLogging = GetDlgItem(hDlg, IDC_STDIN_LOGGING);
	if (hCheckStdinLogging == nullptr) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("GetDlgItem(IDC_STDIN_LOGGING)"));
	}
	SendMessage(hCheckStdinLogging, BM_SETCHECK, setup->IsEnableStdinLogging() ? BST_CHECKED : BST_UNCHECKED, 0);
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
	setup->SetParity(const_cast<SimpleCom::Parity &>(SimpleCom::Parity::values[selected_idx]));

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_STOP_BITS), CB_GETCURSEL, 0, 0));
	if (selected_idx == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("No item is selected"), _T("IDC_STOP_BITS"));
	}
	setup->SetStopBits(const_cast<SimpleCom::StopBits&>(SimpleCom::StopBits::values[selected_idx]));

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_FLOW_CTL), CB_GETCURSEL, 0, 0));
	if (selected_idx == CB_ERR) {
		throw SimpleCom::WinAPIException(_T("No item is selected"), _T("IDC_FLOW_CTL"));
	}
	setup->SetFlowControl(const_cast<SimpleCom::FlowControl&>(SimpleCom::FlowControl::values[selected_idx]));

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

	checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_LOG), BM_GETCHECK, 0, 0);
	if (checked == BST_CHECKED) {
		TCHAR logfile[MAX_PATH];
		auto result = GetDlgItemText(hDlg, IDC_LOGFILEEDIT, logfile, MAX_PATH);
		if ((result == 0) || (result >= MAX_PATH)) {
			throw SimpleCom::SerialSetupException(_T("SimpleCom configuration"), _T("Invalid log file"));
		}
		else if (_tcslen(logfile) == 0) {
			throw SimpleCom::SerialSetupException(_T("SimpleCom configuration"), _T("Log file was not specified in spite of logging enabled."));
		}
		else {
			setup->SetLogFile(logfile);
			setup->SetEnableStdinLogging(SendMessage(GetDlgItem(hDlg, IDC_STDIN_LOGGING), BM_GETCHECK, 0, 0) == BST_CHECKED);
		}
	}
	else {
		setup->SetLogFile(nullptr);
		setup->SetEnableStdinLogging(false);
	}

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
			switch (LOWORD(wParam)) {
			case IDC_CHECK_AUTO_RECONNECT: {
				BOOL checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_AUTO_RECONNECT), BM_GETCHECK, 0, 0) == BST_CHECKED;
				EnableWindow(GetDlgItem(hDlg, IDC_RECONNECT_PAUSE), checked);
				EnableWindow(GetDlgItem(hDlg, IDC_RECONNECT_TIMEOUT), checked);
				return TRUE;
			}
			case IDC_CHECK_LOG: {
				BOOL checked = SendMessage(GetDlgItem(hDlg, IDC_CHECK_LOG), BM_GETCHECK, 0, 0) == BST_CHECKED;
				EnableWindow(GetDlgItem(hDlg, IDC_LOGFILEBTN), checked);
				EnableWindow(GetDlgItem(hDlg, IDC_LOGFILEEDIT), checked);
				EnableWindow(GetDlgItem(hDlg, IDC_STDIN_LOGGING), checked);
				return TRUE;
			}
			case IDC_LOGFILEBTN: {
				OPENFILENAME filename_param = { 0 };
				TCHAR filename[MAX_PATH] = { 0 };
				filename_param.lStructSize = sizeof(OPENFILENAME);
				filename_param.lpstrFile = filename;
				filename_param.nMaxFile = MAX_PATH;
				filename_param.hwndOwner = hDlg;

				if (GetSaveFileName(&filename_param)) {
					if (!SetDlgItemText(hDlg, IDC_LOGFILEEDIT, filename)) {
						throw SimpleCom::WinAPIException(GetLastError(), _T("SetDlgItemText(IDC_LOGFILEEDIT)"));
					}
				}
				return TRUE;
			}
			case IDCONNECT:
				GetConfigurationFromDialog(hDlg, setup);
				EndDialog(hDlg, IDCONNECT);
				return TRUE;
			case IDCANCEL:
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			default:
				return FALSE;
			}

		case WM_CLOSE:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
	}
	catch (SimpleCom::WinAPIException& e) {
		MessageBox(hDlg, e.GetErrorText().c_str(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
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

	Validate();
}

void SimpleCom::SerialSetup::Validate() {
	if (!IsShowDialog() && _port.empty()) {
		throw std::invalid_argument("Serial port is not specified");
	}

	if(IsEnableStdinLogging() && GetLogFile() == nullptr) {
		throw std::invalid_argument("Log file should be configured when stdin logging is enabled");
	}

	if (IsBatchMode()) {
		if (_port.empty()) {
			throw std::invalid_argument("Serial port have to be set with batch mode");
		}
		if(IsShowDialog()) {
			throw std::invalid_argument("Configuration dialog cannot be configured with batch mode");
		}
		if (GetUseTTYResizer()) {
			throw std::invalid_argument("TTY resizer cannot be configured with batch mode");
		}
		if (GetAutoReconnect()) {
			throw std::invalid_argument("Auto reconnect cannot be configured with batch mode");
		}
		if(GetLogFile() != nullptr) {
			throw std::invalid_argument("Logging cannot be configured with batch mode");
		}
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
	dcb->fParity = GetParity() != SimpleCom::Parity::NO_PARITY;
	dcb->Parity = GetParity();
	dcb->fDtrControl = DTR_CONTROL_ENABLE;
	dcb->fRtsControl = RTS_CONTROL_ENABLE;

	if (GetFlowControl() == SimpleCom::FlowControl::HARDWARE) {
		dcb->fOutxCtsFlow = TRUE;
		dcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	else if (GetFlowControl() == SimpleCom::FlowControl::SOFTWARE) {
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
