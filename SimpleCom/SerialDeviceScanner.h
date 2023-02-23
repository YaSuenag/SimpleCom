/*
 * Copyright (C) 2023, Yasumasa Suenaga
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


namespace SimpleCom {

	typedef std::map<TString, TString> TDeviceMap;

	class SerialDeviceScanner
	{
	private:
		HANDLE        _device_scan_event;
		HWND          _parent_hwnd;
		volatile HWND _dialog_hwnd;
		TDeviceMap    _devices;
		TString       _target_port;

	public:
		SerialDeviceScanner(HWND parent_hwnd);
		virtual ~SerialDeviceScanner();

		void WaitSerialDevices(const int period);
		void ScanSerialDevices();

		inline HANDLE GetDeviceScanEvent() {
			return _device_scan_event;
		}

		inline HWND GetParentHwnd() {
			return _parent_hwnd;
		}

		inline void SetDialogHwnd(HWND hwnd) {
			_dialog_hwnd = hwnd;
		}

		inline HWND GetDialogHwnd() {
			return _dialog_hwnd;
		}

		inline TDeviceMap& GetDevices() {
			return _devices;
		}

		inline void SetTargetPort(TString& port) {
			_target_port = port;
		}

		inline TString& GetTargetPort() {
			return _target_port;
		}

	};

	/*
	 * Exception class for serial device scanning excludes Windows API error.
	 * This class wouldn't be manage memory, so caller has responsibility to manage it.
	 */
	class SerialDeviceScanException {
	private:
		LPCTSTR _error_caption;
		LPCTSTR _error_text;

	public:
		SerialDeviceScanException(LPCTSTR error_caption, LPCTSTR error_text) noexcept : _error_caption(error_caption), _error_text(error_text) {};
		virtual ~SerialDeviceScanException() noexcept {};

		inline LPCTSTR GetErrorCaption() noexcept {
			return _error_caption;
		}

		inline LPCTSTR GetErrorText() noexcept {
			return _error_text;
		}

	};
}