#include "stdafx.h"
#include "BatchRedirector.h"

static constexpr int buf_sz = 256;

DWORD WINAPI BatchStdInRedirector(_In_ LPVOID lpParameter) {
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdIn == INVALID_HANDLE_VALUE) {
		std::cerr << "Error: Could not get STDIN handle." << std::endl;
		return -1;
	}

	// Disable line input mode to read raw input from console. (for batch mode without file redirection)
	DWORD mode;
	GetConsoleMode(hStdIn, &mode);
	mode &= ~ENABLE_LINE_INPUT;
	SetConsoleMode(hStdIn, mode);

	HANDLE hSerial = reinterpret_cast<HANDLE>(lpParameter);
	char buf[buf_sz];
	DWORD nBytesRead;
	while(ReadFile(hStdIn, buf, sizeof(buf), &nBytesRead, NULL) && nBytesRead > 0) {
		DWORD nBytesWritten;
		DWORD nBytesRemain = nBytesRead;
		while (nBytesRemain > 0) {
			if (WriteFile(hSerial, &buf[nBytesRead - nBytesRemain], nBytesRemain, &nBytesWritten, nullptr)) {
				nBytesRemain -= nBytesWritten;
			}
		}
	}

	return 0;
}

DWORD WINAPI BatchStdOutRedirector(_In_ LPVOID lpParameter) {
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) {
		std::cerr << "Error: Could not get STDOUT handle." << std::endl;
		return -1;
	}

	HANDLE hSerial = reinterpret_cast<HANDLE>(lpParameter);
	char buf[buf_sz];
	DWORD nBytesRead;
	while (ReadFile(hSerial, buf, sizeof(buf), &nBytesRead, NULL)) {
		DWORD nBytesWritten;
		DWORD nBytesRemain = nBytesRead;
		while (nBytesRemain > 0) {
			if (WriteFile(hStdOut, &buf[nBytesRead - nBytesRemain], nBytesRemain, &nBytesWritten, nullptr)) {
				nBytesRemain -= nBytesWritten;
			}
		}
	}

	return 0;
}

std::tuple<LPTHREAD_START_ROUTINE, LPVOID> SimpleCom::BatchRedirector::GetStdInRedirector() {
	return { &BatchStdInRedirector, _hSerial };
}

std::tuple<LPTHREAD_START_ROUTINE, LPVOID> SimpleCom::BatchRedirector::GetStdOutRedirector() {
	return { &BatchStdOutRedirector, _hSerial };
}