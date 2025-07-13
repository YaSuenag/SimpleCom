/*
 * Copyright (C) 2025, Yasumasa Suenaga
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
#include "BatchRedirector.h"

static void RedirectHandle(HANDLE hSource, HANDLE hDest) {
	char buf[buf_sz];
	DWORD nBytesRead;
	while (ReadFile(hSource, buf, sizeof(buf), &nBytesRead, NULL)) {
		DWORD nBytesWritten;
		DWORD nBytesRemain = nBytesRead;
		while (nBytesRemain > 0) {
			if (WriteFile(hDest, &buf[nBytesRead - nBytesRemain], nBytesRemain, &nBytesWritten, nullptr)) {
				nBytesRemain -= nBytesWritten;
			}
		}
	}
}

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
	RedirectHandle(hStdIn, hSerial);
	return 0;
}

DWORD WINAPI BatchStdOutRedirector(_In_ LPVOID lpParameter) {
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE) {
		std::cerr << "Error: Could not get STDOUT handle." << std::endl;
		return -1;
	}

	HANDLE hSerial = reinterpret_cast<HANDLE>(lpParameter);
	RedirectHandle(hSerial, hStdOut);
	return 0;
}

std::tuple<LPTHREAD_START_ROUTINE, LPVOID> SimpleCom::BatchRedirector::GetStdInRedirector() {
	return { &BatchStdInRedirector, _hSerial };
}

std::tuple<LPTHREAD_START_ROUTINE, LPVOID> SimpleCom::BatchRedirector::GetStdOutRedirector() {
	return { &BatchStdOutRedirector, _hSerial };
}