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
