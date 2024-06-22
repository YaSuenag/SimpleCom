/*
 * Copyright (C) 2023, 2024, Yasumasa Suenaga
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
#include "SerialPortWriter.h"
#include "LogWriter.h"


namespace SimpleCom {

	class SerialConnection
	{
	private:
		TString _device;
		DCB _dcb;
		HWND _parent_hwnd;
		HANDLE _hStdIn;
		HANDLE _hStdOut;
		bool _useTTYResizer;
		LogWriter* _logwriter;
		bool _enableStdinLogging;

		void InitSerialPort(const HANDLE hSerial);
		bool ShouldTerminate(SerialPortWriter& writer, const HANDLE hTermEvent);
		void ProcessKeyEvents(const KEY_EVENT_RECORD keyevent, SerialPortWriter& writer);
		bool StdInRedirector(const HANDLE hSerial, const HANDLE hTermEvent);

	public:
		SerialConnection(TString& device, DCB* dcb, HWND hwnd, HANDLE hStdIn, HANDLE hStdOut, bool useTTYResizer, LPCTSTR logfilename, bool enableStdinLogging);
		virtual ~SerialConnection();

		bool DoSession();
	};

}