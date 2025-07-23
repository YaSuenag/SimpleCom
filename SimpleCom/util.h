/*
 * Copyright (C) 2023, 2025, Yasumasa Suenaga
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
#pragma once

#include "stdafx.h"
#include "WinAPIException.h"

/*
 * RAII for HANDLE
 */
class HandleHandler {
private:
	HANDLE _handle;

public:
	HandleHandler(HANDLE handle, LPCTSTR error_caption) : _handle(handle) {
		if (_handle == INVALID_HANDLE_VALUE) {
			throw SimpleCom::WinAPIException(GetLastError(), error_caption);
		}
	}

	~HandleHandler() {
		if (_handle != INVALID_HANDLE_VALUE) {
			CloseHandle(_handle);
		}
	}

	inline HANDLE handle() const {
		return _handle;
	}
};

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
			throw SimpleCom::WinAPIException(status, lpSubKey);
		}
	}

	~RegistryKeyHandler() {
		if (hKey != INVALID_HANDLE_VALUE) {
			RegCloseKey(hKey);
		}
	}

	HKEY key() const {
		return hKey;
	}
};

/*
 * Class for handling product (SimpleCom) information.
 */
class ProductInfo {
private:
	unsigned char* data;
	DWORD data_len;
	TCHAR exe_path[MAX_PATH];

	void LoadVersionInfo(LPCTSTR exe_name);
	LPCTSTR GetString(LPCTSTR query);

public:
	ProductInfo();
	~ProductInfo();

	LPCTSTR GetProductName();
	LPCTSTR GetProductVersion();
	LPCTSTR GetLegalCopyright();

    inline LPCTSTR GetExePath() const noexcept {
      return exe_path;
    }

};