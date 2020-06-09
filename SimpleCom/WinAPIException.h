#pragma once

#include <Windows.h>
#include <tchar.h>


/*
 * Exception class for Windows API error.
 * This class can hold error code from GetLastError() and caption string for its error.
 * Also it can provides error string from FormatMessage(). Its pointer would be managed by WinAPIException.
 */
class WinAPIException
{
private:
	DWORD _error_code;
	LPCTSTR _error_caption;
	LPTSTR _error_text;

public:
	WinAPIException(DWORD error_code) : WinAPIException(error_code, _T("SimpleCom")) {};
	WinAPIException(DWORD error_code, LPCTSTR error_caption);
	virtual ~WinAPIException();

	inline DWORD GetErrorCode() noexcept {
		return _error_code;
	}

	inline LPCTSTR GetErrorCaption() noexcept {
		return _error_caption;
	}

	inline LPTSTR GetErrorText() noexcept {
		return _error_text;
	}
};

