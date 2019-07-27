#include "WinAPIException.h"



WinAPIException::WinAPIException(DWORD error_code, LPCTSTR error_caption) : _error_code(error_code),
                                                                            _error_caption(error_caption),
	                                                                        _error_text(NULL)
{
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, _error_code, 0, reinterpret_cast<LPTSTR>(&_error_text), 0, NULL);
}


WinAPIException::~WinAPIException()
{
	if (_error_text != NULL) {
		LocalFree(_error_text);
	}
}
