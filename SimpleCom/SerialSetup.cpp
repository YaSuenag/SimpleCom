#include <Windows.h>
#include <tchar.h>

#include "SerialSetup.h"
#include "WinAPIException.h"
#include "resource.h"


#ifdef _UNICODE
#define TO_STRING(x) std::to_wstring(x)
#else
#define TO_STRING(x) std::to_string(x)
#endif


static constexpr Parity NO_PARITY{ NOPARITY, _T("none") };
static constexpr Parity ODD_PARITY{ ODDPARITY, _T("odd") };
static constexpr Parity EVEN_PARITY{ EVENPARITY, _T("even") };
static constexpr Parity MARK_PARITY{ MARKPARITY, _T("mark") };
static constexpr Parity SPACE_PARITY{ SPACEPARITY, _T("space") };
static constexpr Parity parities[] = { NO_PARITY, ODD_PARITY, EVEN_PARITY, MARK_PARITY, SPACE_PARITY };

static constexpr FlowControl NONE{ __COUNTER__, _T("none") };
static constexpr FlowControl HARDWARE{ __COUNTER__, _T("hardware") };
static constexpr FlowControl SOFTWARE{ __COUNTER__, _T("software") };
static constexpr FlowControl flowctrls[] = { NONE, HARDWARE, SOFTWARE };

static constexpr StopBits ONE{ ONESTOPBIT, _T("1") };
static constexpr StopBits ONE5{ ONE5STOPBITS, _T("1.5") };
static constexpr StopBits TWO{ TWOSTOPBITS, _T("2") };
static constexpr StopBits stopbits[] = { ONE, ONE5, TWO };


SerialSetup::SerialSetup() : _port(),
							 _baud_rate(115200),
							 _byte_size(8),
							 _parity(const_cast<Parity&>(parities[0])), // NO__PARITY
							 _stop_bits(const_cast<StopBits&>(stopbits[0])), // ONE
							 _flow_control(const_cast<FlowControl&>(flowctrls[0])), // NONE
							 _devices()
{
	initialize();
}


SerialSetup::~SerialSetup()
{
	// Do nothing
}

void SerialSetup::initialize() {
	HKEY hKey;
	LSTATUS status;

	status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), REG_OPTION_OPEN_LINK, KEY_READ, &hKey);
	if (status != ERROR_SUCCESS) {
		throw WinAPIException(status, _T("HKLM\\HARDWARE\\DEVICEMAP\\SERIALCOMM"));
	}

	DWORD numValues, maxValueNameLen, maxValueLen;
	status = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &numValues, &maxValueNameLen, &maxValueLen, NULL, NULL);
	if (status != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		throw WinAPIException(status, _T("RegQueryInfoKey()"));
	}
	if (numValues <= 0) {
		RegCloseKey(hKey);
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
	RegCloseKey(hKey);
}

static void InitializeDialog(HWND hDlg, SerialSetup *setup) {
	TString text_str;

	HWND hComboSerialDevice = GetDlgItem(hDlg, IDC_SERIAL_DEVICE);
	for (auto itr = setup->GetDevices().begin(); itr != setup->GetDevices().end(); itr++) {
		text_str = itr->first + _T(": ") + itr->second;
		SendMessage(hComboSerialDevice, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text_str.c_str()));
	}
	SendMessage(hComboSerialDevice, CB_SETCURSEL, 0, 0);

	text_str = TO_STRING(setup->GetBaudRate());
	SetDlgItemText(hDlg, IDC_BAUD_RATE, text_str.c_str());

	text_str = TO_STRING(setup->GetByteSize());
	SetDlgItemText(hDlg, IDC_BYTE_SIZE, text_str.c_str());

	HWND hComboParity = GetDlgItem(hDlg, IDC_PARITY);
	for (auto parity : parities) {
		SendMessage(hComboParity, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(parity.tstr()));
	}
	SendMessage(hComboParity, CB_SETCURSEL, 0, 0);


	HWND hComboStopBits = GetDlgItem(hDlg, IDC_STOP_BITS);
	for (auto stopbit : stopbits) {
		SendMessage(hComboStopBits, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(stopbit.tstr()));
	}
	SendMessage(hComboStopBits, CB_SETCURSEL, 0, 0);

	HWND hComboFlowCtl = GetDlgItem(hDlg, IDC_FLOW_CTL);
	for (auto flowctl : flowctrls) {
		SendMessage(hComboFlowCtl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(flowctl.tstr()));
	}
	SendMessage(hComboFlowCtl, CB_SETCURSEL, 0, 0);
}

static bool GetConfigurationFromDialog(HWND hDlg, SerialSetup* setup) {
	int selected_idx;;

	// https://stackoverflow.com/questions/7648756/is-the-order-of-iterating-through-stdmap-known-and-guaranteed-by-the-standard
	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_SERIAL_DEVICE), CB_GETCURSEL, 0, 0));
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
	setup->SetParity(const_cast<Parity &>(parities[selected_idx]));

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_STOP_BITS), CB_GETCURSEL, 0, 0));
	setup->SetStopBits(const_cast<StopBits&>(stopbits[selected_idx]));

	selected_idx = static_cast<int>(SendMessage(GetDlgItem(hDlg, IDC_FLOW_CTL), CB_GETCURSEL, 0, 0));
	setup->SetFlowControl(const_cast<FlowControl&>(flowctrls[selected_idx]));

	return true;
}

static INT_PTR CALLBACK SettingDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	static SerialSetup* setup;
	
	switch (msg) {
		case WM_INITDIALOG:
			setup = reinterpret_cast<SerialSetup*>(lParam);
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

	return FALSE;
}

bool SerialSetup::ShowConfigureDialog(HINSTANCE hInst, HWND hWnd) noexcept {
	return IDCONNECT == DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SETUP), hWnd, &SettingDlgProc, reinterpret_cast<LPARAM>(this));
}

void SerialSetup::SaveToDCB(LPDCB dcb) noexcept {
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
