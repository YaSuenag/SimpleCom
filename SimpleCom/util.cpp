/*
 * Copyright (C) 2024, 2025, Yasumasa Suenaga
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

#include "util.h"


ProductInfo::ProductInfo() {
	GetModuleFileName(NULL, exe_path, MAX_PATH);
	if (GetLastError() == ERROR_SUCCESS) {
		LoadVersionInfo(exe_path);
	}
	else {
		data = nullptr;
		data_len = 0;
	}
}

ProductInfo::~ProductInfo() {
	if (data_len > 0) {
		free(data);
	}
}

void ProductInfo::LoadVersionInfo(LPCTSTR exe_name) {
	DWORD dummy = 0;
	data_len = GetFileVersionInfoSize(exe_name, &dummy);
	if (data_len == 0) {
		data = nullptr;
		return;
	}

	data = (unsigned char *)malloc(data_len);
	if (data == nullptr) {
		data_len = 0;
		return;
	}

	if (!GetFileVersionInfo(exe_name, 0, data_len, data)) {
		free(data);
		data = nullptr;
		data_len = 0;
		return;
	}
}

LPCTSTR ProductInfo::GetString(LPCTSTR query) {
	if (data_len > 0) {
		LPTSTR name = nullptr;
		UINT nameLen;
		if (VerQueryValue(data, query, reinterpret_cast<LPVOID*>(&name), &nameLen)) {
			return name;
		}
	}
	return nullptr;
}

LPCTSTR ProductInfo::GetProductName() {
	// Neutral codepage (000004b0)
	LPCTSTR query_result = GetString(_T("\\StringFileInfo\\000004b0\\ProductName"));
	return query_result ? query_result : _T("");
}

LPCTSTR ProductInfo::GetProductVersion() {
	// Neutral codepage (000004b0)
	LPCTSTR query_result = GetString(_T("\\StringFileInfo\\000004b0\\ProductVersion"));
	return query_result ? query_result : _T("");
}

LPCTSTR ProductInfo::GetLegalCopyright() {
	// Neutral codepage (000004b0)
	LPCTSTR query_result = GetString(_T("\\StringFileInfo\\000004b0\\LegalCopyright"));
	return query_result ? query_result : _T("Copyright (C) Yasumasa Suenaga");
}