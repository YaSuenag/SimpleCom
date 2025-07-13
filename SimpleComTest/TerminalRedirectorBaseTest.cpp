/*
 * Copyright (C) 2025, Yasumasa Suenaga
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

#include "TerminalRedirectorBase.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static constexpr DWORD stdin_redirector_result = 100;
static constexpr DWORD stdout_redirector_result = 200;

static DWORD WINAPI RedirectorTestThreadEntry(_In_ LPVOID lpParameter) {
	return reinterpret_cast<DWORD>(lpParameter);
}

namespace SimpleComTest
{

	class TestRedirector : public SimpleCom::TerminalRedirectorBase
	{
	public:
		TestRedirector() : SimpleCom::TerminalRedirectorBase(INVALID_HANDLE_VALUE) {}

		virtual std::tuple<LPTHREAD_START_ROUTINE, LPVOID> GetStdInRedirector() override
		{
			return { &RedirectorTestThreadEntry, reinterpret_cast<LPVOID>(stdin_redirector_result) };
		}

		virtual std::tuple<LPTHREAD_START_ROUTINE, LPVOID> GetStdOutRedirector() override
		{
			return { &RedirectorTestThreadEntry, reinterpret_cast<LPVOID>(stdout_redirector_result) };
		}

		DWORD GetStdInRedirectorResult()
		{
			DWORD result;
			if (!GetExitCodeThread(_hThreadStdIn, &result)) {
				result = -1;
			}
			return result;
		}

		DWORD GetStdOutRedirectorResult()
		{
			DWORD result;
			if (!GetExitCodeThread(_hThreadStdOut, &result)) {
				result = -1;
			}
			return result;
		}

	};

	TEST_CLASS(TerminalRedirectorBaseTest)
	{
	public:

		TEST_METHOD(RedirectorTest)
		{
			TestRedirector redirector;
			redirector.StartRedirector();
			redirector.AwaitTermination();

			Assert::AreEqual(stdin_redirector_result, redirector.GetStdInRedirectorResult());
			Assert::AreEqual(stdout_redirector_result, redirector.GetStdOutRedirectorResult());
		}

		TEST_METHOD(ReattachableTest) {
			TestRedirector redirector;
			Assert::IsFalse(redirector.Reattachable());
		}

	};

}