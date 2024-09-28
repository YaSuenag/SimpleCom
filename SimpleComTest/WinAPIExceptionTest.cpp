/*
 * Copyright (C) 2023, 2024, Yasumasa Suenaga
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
			Assert::IsTrue(e.GetErrorText().find(expected) != TString::npos);
			Assert::IsTrue(e.GetErrorText().find(_T("(0x2)")) != TString::npos);  // ERROR_FILE_NOT_FOUND in hex
		}

	};
}
