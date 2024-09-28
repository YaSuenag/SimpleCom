/*
 * Copyright (C) 2024, Yasumasa Suenaga
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

#include "LogWriter.h"
#include "WinAPIException.h"


constexpr LPCTSTR TESTFILENAME = _T("test.log");

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SimpleComTest
{
	TEST_CLASS(LogWriterTest)
	{
	private:

		void test_log_contents(const char* expected) {
			HANDLE hnd = CreateFile(TESTFILENAME, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hnd == INVALID_HANDLE_VALUE) {
				SimpleCom::WinAPIException ex(GetLastError());
				Assert::Fail(ex.GetErrorText().c_str());
			}
			DWORD fsize = GetFileSize(hnd, nullptr);
			char* contents = new char[fsize + 1];
			ZeroMemory(contents, fsize + 1);

			ReadFile(hnd, contents, fsize, nullptr, nullptr);

			Assert::AreEqual(expected, contents);

			delete[] contents;
			CloseHandle(hnd);
		}

	public:

		TEST_METHOD_CLEANUP(Cleanup) {
			DeleteFile(TESTFILENAME);
		}

		TEST_METHOD(WriteCharTest)
		{
			SimpleCom::LogWriter writer(TESTFILENAME);
			test_log_contents("");

			writer.Write('1');
			test_log_contents("1");
		}

		TEST_METHOD(WriteStringTest)
		{
			SimpleCom::LogWriter writer(TESTFILENAME);

			writer.Write("abc", 3);
			test_log_contents("abc");
		}

		TEST_METHOD(AppendTest)
		{
			// First time log writing
			{
				SimpleCom::LogWriter writer(TESTFILENAME);

				writer.Write("abc", 3);
				test_log_contents("abc");
			}

			// Second time log writing
			{
				SimpleCom::LogWriter writer(TESTFILENAME);

				writer.Write("xyz", 3);
				test_log_contents("abcxyz");
			}
		}

	};
}
