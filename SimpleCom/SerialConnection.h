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
#include "LogWriter.h"


namespace SimpleCom {

	class SerialConnection
	{
	private:
		TString _device;
		DCB _dcb;
		LogWriter* _logwriter;
		bool _enableStdinLogging;

		void InitSerialPort(const HANDLE hSerial);

	public:
		SerialConnection(TString& device, DCB* dcb, LPCTSTR logfilename, bool enableStdinLogging);
		SerialConnection(TString& device, DCB* dcb) : SerialConnection(device, dcb, nullptr, false) {};
		virtual ~SerialConnection() {};

		bool DoSession(bool allowDetachDevice, bool useTTYResizer, HWND parent_hwnd);
		void DoBatch();
	};

}