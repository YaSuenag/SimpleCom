/*
 * Copyright (C) 2019, 2023, Yasumasa Suenaga
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
#include "SerialDeviceScanner.h"

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

	/* Type for using CommandlineOption for "--help" */
	typedef struct { bool dummy; } HelpSwitch;

	/* Forward declaration */
	class SerialSetup;

	class CommandlineOptionBase {
	protected:
		SerialSetup* _setup;
		LPCTSTR _commandline_option;
		LPCTSTR _args;
		LPCTSTR _description;

	public:
		CommandlineOptionBase(SerialSetup* setup, LPCTSTR option, LPCTSTR args, LPCTSTR description) : _setup(setup), _commandline_option(option), _args(args), _description(description) {}
		~CommandlineOptionBase() {}

		LPCTSTR GetCommandlineOption() { return _commandline_option;  }
		LPCTSTR GetArgs() { return _args; }
		LPCTSTR GetDescription() { return _description; }

		virtual bool need_arguments() = 0;
		virtual void set_from_arg(LPCTSTR arg) = 0;
	};

	template <typename T> class CommandlineOption : public CommandlineOptionBase {
	private:
		T _value;

	public:
		CommandlineOption(SerialSetup* setup, LPCTSTR option, LPCTSTR args, LPCTSTR description, T default_val) : CommandlineOptionBase(setup, option, args, description), _value(default_val) {}
		~CommandlineOption() {}
		void set(T new_value) { _value = new_value; }
		T get() { return _value; }

		bool need_arguments() { return !std::is_same<T, bool>::value && !std::is_same<T, HelpSwitch>::value; }
        void set_from_arg(LPCTSTR arg);
	};

	/*
	 * This class shows setup dialog box for serial connection.
	 * The caller can retrieve serial configuration which the user sets on dialog box as `LPDCB`.
	 */
	class SerialSetup
	{
	private:
		TString     _port;
		CommandlineOption<DWORD>       _baud_rate;
		CommandlineOption<BYTE>        _byte_size;
		CommandlineOption<Parity>      _parity;
		CommandlineOption<StopBits>    _stop_bits;
		CommandlineOption<FlowControl> _flow_control;
		CommandlineOption<bool>        _use_utf8;
		CommandlineOption<bool>        _show_dialog;
		CommandlineOption<int>         _wait_device_period;
		CommandlineOption<bool>        _auto_reconnect;
		CommandlineOption<int>         _auto_reconnect_pause_in_sec;
		CommandlineOption<int>         _auto_reconnect_timeout_in_sec;
		CommandlineOption<HelpSwitch>  _help;
		SerialDeviceScanner _scanner;

		friend class CommandlineOption<HelpSwitch>;
		std::map<TString, CommandlineOptionBase*> _options;

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
			_baud_rate.set(baud_rate);
		}

		inline DWORD GetBaudRate() {
			return _baud_rate.get();
		}

		inline void SetByteSize(const BYTE byte_size) {
			_byte_size.set(byte_size);
		}

		inline BYTE GetByteSize() {
			return _byte_size.get();
		}

		inline void SetParity(Parity& parity) {
			_parity.set(parity);
		}

		inline Parity GetParity() {
			return _parity.get();
		}

		inline void SetStopBits(StopBits& stop_bits) {
			_stop_bits.set(stop_bits);
		}

		inline StopBits GetStopBits() {
			return _stop_bits.get();
		}

		inline void SetFlowControl(FlowControl& flow_control) {
			_flow_control.set(flow_control);
		}

		inline FlowControl GetFlowControl() {
			return _flow_control.get();
		}

		inline void SetUseUTF8(bool enabled) {
			_use_utf8.set(enabled);
		}

		inline bool GetUseUTF8() {
			return _use_utf8.get();
		}

		inline void SetShowDialog(bool value) {
			_show_dialog.set(value);
		}

		inline bool IsShowDialog() {
			return _show_dialog.get();
		}

		inline int GetWaitDevicePeriod() {
			return _wait_device_period.get();
		}

		inline void SetAutoReconnect(bool enabled) {
			_auto_reconnect.set(enabled);
		}

		inline bool GetAutoReconnect() {
			return _auto_reconnect.get();
		}

		inline void SetAutoReconnectPauseInSec(int sec) {
			_auto_reconnect_pause_in_sec.set(sec);
		}

		inline int GetAutoReconnectPauseInSec() {
			return _auto_reconnect_pause_in_sec.get();
		}

		inline void SetAutoReconnectTimeoutInSec(int sec) {
			_auto_reconnect_timeout_in_sec.set(sec);
		}

		inline int GetAutoReconnectTimeoutInSec() {
			return _auto_reconnect_timeout_in_sec.get();
		}

		inline SerialDeviceScanner& GetDeviceScanner() {
			return _scanner;
		}

		template <typename T> CommandlineOption<T> GetOption() {
			return _baud_rate;
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