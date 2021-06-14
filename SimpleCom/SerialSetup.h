/*
 * Copyright (C) 2019, 2021, Yasumasa Suenaga
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

#include "EnumValue.h"

namespace SimpleCom {

	/* Enum for parity */
	class Parity : public EnumValue {
	public:
		constexpr explicit Parity(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
	};

	/* Enum for flow control */
	class FlowControl : public EnumValue {
	public:
		constexpr explicit FlowControl(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
	};

	/* Enum for stop bits */
	class StopBits : public EnumValue {
	public:
		constexpr explicit StopBits(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
	};

#ifdef  UNICODE
	typedef std::wstring TString;
#else   /* UNICODE */
	typedef std::string TString;
#endif /* UNICODE */

	typedef std::map<TString, TString> TDeviceMap;

	/*
	 * This class shows setup dialog box for serial connection.
	 * The caller can retrieve serial configuration which the user sets on dialog box as `LPDCB`.
	 */
	class SerialSetup
	{
	private:
		TString     _port;
		DWORD       _baud_rate;
		BYTE        _byte_size;
		Parity      _parity;
		StopBits    _stop_bits;
		FlowControl _flow_control;
		TDeviceMap  _devices;

		void initialize();

	public:
		SerialSetup();
		virtual ~SerialSetup();

		inline void SetPort(const TString& port) {
			_port = port;
		}

		inline TString& GetPort() {
			return _port;
		}

		inline void SetBaudRate(const DWORD baud_rate) {
			_baud_rate = baud_rate;
		}

		inline DWORD GetBaudRate() {
			return _baud_rate;
		}

		inline void SetByteSize(const BYTE byte_size) {
			_byte_size = byte_size;
		}

		inline BYTE GetByteSize() {
			return _byte_size;
		}

		inline void SetParity(Parity& parity) {
			_parity = parity;
		}

		inline Parity& GetParity() {
			return _parity;
		}

		inline void SetStopBits(StopBits& stop_bits) {
			_stop_bits = stop_bits;
		}

		inline StopBits& GetStopBits() {
			return _stop_bits;
		}

		inline void SetFlowControl(FlowControl& flow_control) {
			_flow_control = flow_control;
		}

		inline FlowControl& GetFlowControl() {
			return _flow_control;
		}

		inline TDeviceMap& GetDevices() {
			return _devices;
		}

		bool ShowConfigureDialog(HINSTANCE hInst, HWND hWnd) noexcept;
		void ParseArguments(int argc, LPCTSTR argv[]);
		void SaveToDCB(LPDCB dcb) noexcept;
	};

	/*
	 * Exception class for serial setup excepting Windows API error.
	 * This class wouldn't be manage memory, so caller has responsibility to manage it.
	 */
	class SerialSetupException {
	private:
		LPCTSTR _error_caption;
		LPCTSTR _error_text;

	public:
		SerialSetupException(LPCTSTR error_caption, LPCTSTR error_text) noexcept : _error_caption(error_caption), _error_text(error_text) {};
		virtual ~SerialSetupException() noexcept {};

		inline LPCTSTR GetErrorCaption() noexcept {
			return _error_caption;
		}

		inline LPCTSTR GetErrorText() noexcept {
			return _error_text;
		}

	};

}