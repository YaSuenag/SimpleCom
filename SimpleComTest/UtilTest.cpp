#include "pch.h"
#include "CppUnitTest.h"

#include "util.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SimpleComTest
{
	TEST_CLASS(UtilmTest)
	{
	public:

		TEST_METHOD(HandleHandlerTest)
		{
			HANDLE handle = INVALID_HANDLE_VALUE;

			{
				HandleHandler handler(CreateFile(_T("test.dat"), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL), _T("unexpected"));
				handle = handler.handle();
				Assert::AreNotEqual(INVALID_HANDLE_VALUE, handle);
			}

			CloseHandle(handle);
			Assert::AreEqual((DWORD)ERROR_INVALID_HANDLE, GetLastError());
		}

		TEST_METHOD(RegistryKeyHandlerTest)
		{
			RegistryKeyHandler handler(HKEY_LOCAL_MACHINE, _T(R"(SOFTWARE\Microsoft\Windows\Windows Error Reporting)"), REG_OPTION_OPEN_LINK, KEY_READ);
			HKEY key = handler.key();
			Assert::AreNotEqual(INVALID_HANDLE_VALUE, (HANDLE)key);
		}

	};
}
