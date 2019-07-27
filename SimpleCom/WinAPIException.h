#pragma once

#include <Windows.h>
#include <tchar.h>


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

