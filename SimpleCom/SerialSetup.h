#pragma once

#include <Windows.h>
#include <tchar.h>

#include <map>
#include <string>

#include "EnumValue.h"


class Parity : public EnumValue {
public:
	constexpr explicit Parity(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
};

class FlowControl : public EnumValue {
public:
	constexpr explicit FlowControl(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
};

class StopBits : public EnumValue {
public:
	constexpr explicit StopBits(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
};


typedef std::basic_string<TCHAR> TString;
typedef std::map<TString, TString> TDeviceMap;

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
	void SaveToDCB(LPDCB dcb) noexcept;
};

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

