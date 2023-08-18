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

	/* Forward declaration */
	class SerialSetup;

	class CommandlineOptionBase {
	protected:
		LPCTSTR _args;
		LPCTSTR _description;

	public:
		CommandlineOptionBase( LPCTSTR args, LPCTSTR description) : _args(args), _description(description) {}
		virtual ~CommandlineOptionBase() {}

		LPCTSTR GetArgs() { return _args; }
		LPCTSTR GetDescription() { return _description; }

		virtual bool need_arguments() = 0;
		virtual void set_from_arg(LPCTSTR arg) = 0;
	};

	template <typename T> class CommandlineOption : public CommandlineOptionBase {
	private:
		T _value;

	public:
		CommandlineOption(LPCTSTR args, LPCTSTR description, T default_val) : CommandlineOptionBase(args, description), _value(default_val) {}
		virtual ~CommandlineOption() {}
		void set(T new_value) { _value = new_value; }
		T get() { return _value; }

		virtual bool need_arguments() { return !std::is_same<T, bool>::value; }
        virtual void set_from_arg(LPCTSTR arg);
	};

	class CommandlineHelpOption : public CommandlineOption<bool> {
	private:
		std::map<TString, CommandlineOptionBase*> *_options;

	public:
		CommandlineHelpOption(std::map<TString, CommandlineOptionBase*> *options) : CommandlineOption<bool>(_T(""), _T("Show this help message"), false), _options(options) {};
		virtual ~CommandlineHelpOption() {};

		bool need_arguments() { return false; }
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
		SerialDeviceScanner _scanner;
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
			static_cast<CommandlineOption<DWORD>*>(_options[_T("--baud-rate")])->set(baud_rate);
		}

		inline DWORD GetBaudRate() {
			return static_cast<CommandlineOption<DWORD>*>(_options[_T("--baud-rate")])->get();
		}

		inline void SetByteSize(const BYTE byte_size) {
			static_cast<CommandlineOption<BYTE>*>(_options[_T("--byte-size")])->set(byte_size);
		}

		inline BYTE GetByteSize() {
			return static_cast<CommandlineOption<BYTE>*>(_options[_T("--byte-size")])->get();
		}

		inline void SetParity(Parity& parity) {
			static_cast<CommandlineOption<Parity>*>(_options[_T("--parity")])->set(parity);
		}

		inline Parity GetParity() {
			return static_cast<CommandlineOption<Parity>*>(_options[_T("--parity")])->get();
		}

		inline void SetStopBits(StopBits& stop_bits) {
			static_cast<CommandlineOption<StopBits>*>(_options[_T("--stop-bits")])->set(stop_bits);
		}

		inline StopBits GetStopBits() {
			return static_cast<CommandlineOption<StopBits>*>(_options[_T("--stop-bits")])->get();
		}

		inline void SetFlowControl(FlowControl& flow_control) {
			static_cast<CommandlineOption<FlowControl>*>(_options[_T("--flow-control")])->set(flow_control);
		}

		inline FlowControl GetFlowControl() {
			return static_cast<CommandlineOption<FlowControl>*>(_options[_T("--flow-control")])->get();
		}

		inline void SetUseUTF8(bool enabled) {
			static_cast<CommandlineOption<bool>*>(_options[_T("--utf8")])->set(enabled);
		}

		inline bool GetUseUTF8() {
			return static_cast<CommandlineOption<bool>*>(_options[_T("--utf8")])->get();
		}

		inline void SetUseTTYResizer(bool enabled) {
			static_cast<CommandlineOption<bool>*>(_options[_T("--tty-resizer")])->set(enabled);
		}

		inline bool GetUseTTYResizer() {
			return static_cast<CommandlineOption<bool>*>(_options[_T("--tty-resizer")])->get();
		}

		inline void SetShowDialog(bool value) {
			static_cast<CommandlineOption<bool>*>(_options[_T("--show-dialog")])->set(value);
		}

		inline bool IsShowDialog() {
			return static_cast<CommandlineOption<bool>*>(_options[_T("--show-dialog")])->get();
		}

		inline int GetWaitDevicePeriod() {
			return static_cast<CommandlineOption<int>*>(_options[_T("--wait-serial-device")])->get();
		}

		inline void SetAutoReconnect(bool enabled) {
			static_cast<CommandlineOption<bool>*>(_options[_T("--auto-reconnect")])->set(enabled);
		}

		inline bool GetAutoReconnect() {
			return static_cast<CommandlineOption<bool>*>(_options[_T("--auto-reconnect")])->get();
		}

		inline void SetAutoReconnectPauseInSec(int sec) {
			static_cast<CommandlineOption<int>*>(_options[_T("--auto-reconnect-pause")])->set(sec);
		}

		inline int GetAutoReconnectPauseInSec() {
			return static_cast<CommandlineOption<int>*>(_options[_T("--auto-reconnect-pause")])->get();
		}

		inline void SetAutoReconnectTimeoutInSec(int sec) {
			static_cast<CommandlineOption<int>*>(_options[_T("--auto-reconnect-timeout")])->set(sec);
		}

		inline int GetAutoReconnectTimeoutInSec() {
			return static_cast<CommandlineOption<int>*>(_options[_T("--auto-reconnect-timeout")])->get();
		}

		inline SerialDeviceScanner& GetDeviceScanner() {
			return _scanner;
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