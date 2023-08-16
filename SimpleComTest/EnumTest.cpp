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

#include "EnumValue.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SimpleComTest
{
	class TestEnum : public SimpleCom::EnumValue {
	public:
		constexpr explicit TestEnum(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};
	};

	TEST_CLASS(SimpleComEnumTest)
	{

	private:
		static constexpr TestEnum A{ 100, _T("A") };
		static constexpr TestEnum A_CLONE{ 100, _T("A-clone") };
		static constexpr TestEnum B{ 200, _T("B") };

	public:

		TEST_METHOD(GetEnumValue)
		{
			Assert::AreEqual(100, (int)A);
		}

		TEST_METHOD(CompareEnum)
		{
			Assert::IsTrue(A == A_CLONE);
			Assert::IsFalse(A == B);
		}

		TEST_METHOD(GetEnumString)
		{
			Assert::AreEqual(A.tstr(), _T("A"));
		}

	};
}
