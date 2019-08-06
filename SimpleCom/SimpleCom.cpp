#include <Windows.h>
#include <conio.h>

#include <iostream>

#include "SerialSetup.h"
#include "WinAPIException.h"

#define ARROW_UP    0x48
#define ARROW_LEFT  0x4b
#define ARROW_RIGHT 0x4d
#define ARROW_DOWN  0x50

#define ESC 0x1b;


static HANDLE stdoutRedirectorThread;

static HANDLE hSerial;
static OVERLAPPED serialReadOverlapped = { 0 };


static void SetVTConsole(HANDLE hConsole) {
	BOOL result;
	DWORD mode;

	result = GetConsoleMode(hConsole, &mode);
	if (result) {
		SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
}

static bool process_arrow(char* data) {
	data[0] = ESC;
	data[1] = '[';

	switch (_getch()) {
	case ARROW_UP:
		data[2] = 'A';
		return true;

	case ARROW_DOWN:
		data[2] = 'B';
		return true;

	case ARROW_RIGHT:
		data[2] = 'C';
		return true;

	case ARROW_LEFT:
		data[2] = 'D';
		return true;

	default:
		return false;
	}

}

static void StdInRedirector(HWND parent_hwnd) {
	OVERLAPPED overlapped = { 0 };
	char data[4];
	int data_len;

	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (overlapped.hEvent == NULL) {
		WinAPIException ex(GetLastError(), _T("SimpleCom"));
		MessageBox(parent_hwnd, ex.GetErrorText(), ex.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return;
	}

	while (true) {
		int ch = _getch();
		if (ch == 0xe0) {
			if (!process_arrow(data)) {
				continue;
			}
			data_len = 3;
		}
		else if ((ch == 0x0) && (_getch() == 0x3b)) { // F1 key
			if (MessageBox(parent_hwnd, _T("Do you want to leave from this serial session?"), _T("SimpleCom"), MB_YESNO | MB_ICONQUESTION) == IDYES){
				break;
			}
			else {
				continue;
			}
		}
		else {
			data[0] = ch;
			data_len = 1;
		}

		ResetEvent(overlapped.hEvent);
		DWORD nBytesWritten;
		if (!WriteFile(hSerial, data, data_len, &nBytesWritten, &overlapped)) {
			if (GetLastError() == ERROR_IO_PENDING) {
				if (!GetOverlappedResult(hSerial, &overlapped, &nBytesWritten, TRUE)) {
					break;
				}
			}
		}

	}

	CloseHandle(overlapped.hEvent);
}

DWORD WINAPI StdOutRedirector(_In_ LPVOID lpParameter) {
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	char buf;
	DWORD nBytesRead;

	while (true) {
		ResetEvent(serialReadOverlapped.hEvent);
		if (!ReadFile(hSerial, &buf, 1, &nBytesRead, &serialReadOverlapped)) {
			if (GetLastError() == ERROR_IO_PENDING) {
				if (!GetOverlappedResult(hSerial, &serialReadOverlapped, &nBytesRead, TRUE)) {
					break;
				}
			}
		}

		if (nBytesRead > 0) {
			DWORD nBytesWritten;
			WriteFile(hStdOut, &buf, nBytesRead, &nBytesWritten, NULL);
		}

	}

	return 0;
}

static HWND GetParentWindow() {
	HWND current = GetConsoleWindow();

	while (true) {
		HWND parent = GetParent(current);

		if (parent == NULL) {
			TCHAR window_text[MAX_PATH] = { 0 };
			GetWindowText(current, window_text, MAX_PATH);

			// If window_text is empty, SimpleCom might be run on Windows Terminal (Could not get valid owner window text)
			return (window_text[0] == 0) ? NULL : current;
		}
		else {
			current = parent;
		}

	}

}

int main()
{
	DCB dcb;
	TString device;
	HWND parent_hwnd = GetParentWindow();

	try {
		SerialSetup setup;
		if (!setup.ShowConfigureDialog(NULL, parent_hwnd)) {
			return -1;
		}
		device = _T("\\\\.\\") + setup.GetPort();
		setup.SaveToDCB(&dcb);
	}
	catch (WinAPIException e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -1;
	}
	catch (SerialSetupException e) {
		MessageBox(parent_hwnd, e.GetErrorText(), e.GetErrorCaption(), MB_OK | MB_ICONERROR);
		return -2;
	}

	hSerial = CreateFile(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hSerial == INVALID_HANDLE_VALUE) {
		WinAPIException e(GetLastError(), NULL);
		MessageBox(parent_hwnd, e.GetErrorText(), _T("Open serial connection"), MB_OK | MB_ICONERROR);
		return -4;
	}

	TString title = _T("SimpleCom: ") + device;
	SetConsoleTitle(title.c_str());

	SetCommState(hSerial, &dcb);
	PurgeComm(hSerial, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	SetCommMask(hSerial, EV_RXCHAR);
	SetupComm(hSerial, 1, 1);

	serialReadOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (serialReadOverlapped.hEvent == NULL) {
		WinAPIException ex(GetLastError(), _T("SimpleCom"));
		MessageBox(parent_hwnd, ex.GetErrorText(), ex.GetErrorCaption(), MB_OK | MB_ICONERROR);
		CloseHandle(hSerial);
		return -1;
	}

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetVTConsole(hStdOut);

	stdoutRedirectorThread = CreateThread(NULL, 0, &StdOutRedirector, NULL, 0, NULL);
	if (stdoutRedirectorThread == NULL) {
		WinAPIException ex(GetLastError(), _T("SimpleCom"));
		MessageBox(parent_hwnd, ex.GetErrorText(), ex.GetErrorCaption(), MB_OK | MB_ICONERROR);
		CloseHandle(hSerial);
		return -2;
	}

	StdInRedirector(parent_hwnd);
	CancelIoEx(hSerial, &serialReadOverlapped);
	WaitForSingleObject(stdoutRedirectorThread, INFINITE);

	CloseHandle(hSerial);
	CloseHandle(serialReadOverlapped.hEvent);

	return 0;
}
