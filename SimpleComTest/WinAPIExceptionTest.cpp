#include "pch.h"
#include "CppUnitTest.h"

#include "WinAPIException.h"

constexpr DWORD TEST_ERRCODE = ERROR_FILE_NOT_FOUND;


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SimpleComTest
{
	TEST_CLASS(WinAPIExceptionTest)
	{
	private:
		LPTSTR expected;

	public:

		TEST_METHOD_INITIALIZE(MessageTestInitialize) {
			DWORD result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, TEST_ERRCODE, 0, reinterpret_cast<LPTSTR>(&expected), 0, NULL);
			if (result == 0) {
				Assert::Fail(_T("Could not create expected string from FormatMessage()"));
			}
		}

		TEST_METHOD_CLEANUP(MessageTestCleanup) {
			LocalFree(expected);
		}

		TEST_METHOD(MessageTest)
		{
			SimpleCom::WinAPIException e(TEST_ERRCODE);
			Assert::AreEqual(expected, e.GetErrorText());
		}

	};
}
