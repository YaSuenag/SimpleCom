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
#include "TerminalRedirectorBase.h"
#include "WinAPIException.h"


void SimpleCom::TerminalRedirectorBase::StartRedirector()
{
	auto [StdInRedirector, stdin_param] = GetStdInRedirector();
	auto [StdOutRedirector, stdout_param] = GetStdOutRedirector();

	_hThreadStdIn = CreateThread(NULL, 0, StdInRedirector, stdin_param, 0, NULL);
	if (GetLastError() != ERROR_SUCCESS) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("CreateThread for StdInRedirector"));
	}

	_hThreadStdOut = CreateThread(NULL, 0, StdOutRedirector, stdout_param, 0, NULL);
	if (GetLastError() != ERROR_SUCCESS) {
		throw SimpleCom::WinAPIException(GetLastError(), _T("CreateThread for StdOutRedirector"));
	}
}

void SimpleCom::TerminalRedirectorBase::AwaitTermination()
{
	HANDLE objs[] = { _hThreadStdIn, _hThreadStdOut };
	WaitForMultipleObjects(sizeof(objs) / sizeof(HANDLE), objs, TRUE, INFINITE);
}

bool SimpleCom::TerminalRedirectorBase::Reattachable()
{
	return false;
}