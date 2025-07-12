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
#pragma once

#include "stdafx.h"
#include "LogWriter.h"
#include "WinAPIException.h"


namespace SimpleCom
{
	class TerminalRedirectorBase
	{
	protected:
		HANDLE _hSerial;
		HANDLE _hThreadStdIn;
		HANDLE _hThreadStdOut;

		virtual std::tuple<LPTHREAD_START_ROUTINE, LPVOID> GetStdInRedirector() = 0;
		virtual std::tuple<LPTHREAD_START_ROUTINE, LPVOID> GetStdOutRedirector() = 0;

	public:
		TerminalRedirectorBase(HANDLE hSerial) : _hSerial(hSerial), _hThreadStdIn(INVALID_HANDLE_VALUE), _hThreadStdOut(INVALID_HANDLE_VALUE) {};
		virtual ~TerminalRedirectorBase() {};

		virtual void StartRedirector();

		// Returns true if the peripheral is reattachable.
		virtual void AwaitTermination();

		virtual bool Reattachable();
	};
}