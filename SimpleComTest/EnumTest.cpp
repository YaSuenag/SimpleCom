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

	TEST_CLASS(ParityTest) {
	public:

		TEST_METHOD(Elements)
		{
			Assert::AreEqual(NOPARITY, static_cast<int>(SimpleCom::Parity::NO_PARITY));
			Assert::AreEqual(_T("none"), SimpleCom::Parity::NO_PARITY.tstr());
			Assert::AreEqual(ODDPARITY, static_cast<int>(SimpleCom::Parity::ODD_PARITY));
			Assert::AreEqual(_T("odd"), SimpleCom::Parity::ODD_PARITY.tstr());
			Assert::AreEqual(EVENPARITY, static_cast<int>(SimpleCom::Parity::EVEN_PARITY));
			Assert::AreEqual(_T("even"), SimpleCom::Parity::EVEN_PARITY.tstr());
			Assert::AreEqual(MARKPARITY, static_cast<int>(SimpleCom::Parity::MARK_PARITY));
			Assert::AreEqual(_T("mark"), SimpleCom::Parity::MARK_PARITY.tstr());
			Assert::AreEqual(SPACEPARITY, static_cast<int>(SimpleCom::Parity::SPACE_PARITY));
			Assert::AreEqual(_T("space"), SimpleCom::Parity::SPACE_PARITY.tstr());
		}

		TEST_METHOD(Compare)
		{
			Assert::IsTrue(SimpleCom::Parity::NO_PARITY == SimpleCom::Parity::NO_PARITY);
			Assert::IsFalse(SimpleCom::Parity::NO_PARITY == SimpleCom::Parity::ODD_PARITY);
		}

		TEST_METHOD(Values)
		{
			Assert::AreEqual(static_cast<size_t>(5), SimpleCom::Parity::values.size());
			Assert::IsTrue(SimpleCom::Parity::NO_PARITY == SimpleCom::Parity::values[0]);
			Assert::IsTrue(SimpleCom::Parity::ODD_PARITY == SimpleCom::Parity::values[1]);
			Assert::IsTrue(SimpleCom::Parity::EVEN_PARITY == SimpleCom::Parity::values[2]);
			Assert::IsTrue(SimpleCom::Parity::MARK_PARITY == SimpleCom::Parity::values[3]);
			Assert::IsTrue(SimpleCom::Parity::SPACE_PARITY == SimpleCom::Parity::values[4]);
		}

	};

	TEST_CLASS(FlowControlTest) {
	public:

		TEST_METHOD(Elements)
		{
			Assert::AreEqual(0, static_cast<int>(SimpleCom::FlowControl::NONE));
			Assert::AreEqual(_T("none"), SimpleCom::FlowControl::NONE.tstr());
			Assert::AreEqual(1, static_cast<int>(SimpleCom::FlowControl::HARDWARE));
			Assert::AreEqual(_T("hardware"), SimpleCom::FlowControl::HARDWARE.tstr());
			Assert::AreEqual(2, static_cast<int>(SimpleCom::FlowControl::SOFTWARE));
			Assert::AreEqual(_T("software"), SimpleCom::FlowControl::SOFTWARE.tstr());
		}

		TEST_METHOD(Compare)
		{
			Assert::IsTrue(SimpleCom::FlowControl::NONE == SimpleCom::FlowControl::NONE);
			Assert::IsFalse(SimpleCom::FlowControl::NONE == SimpleCom::FlowControl::HARDWARE);
		}

		TEST_METHOD(Values)
		{
			Assert::AreEqual(static_cast<size_t>(3), SimpleCom::FlowControl::values.size());
			Assert::IsTrue(SimpleCom::FlowControl::NONE == SimpleCom::FlowControl::values[0]);
			Assert::IsTrue(SimpleCom::FlowControl::HARDWARE == SimpleCom::FlowControl::values[1]);
			Assert::IsTrue(SimpleCom::FlowControl::SOFTWARE == SimpleCom::FlowControl::values[2]);
		}

	};

	TEST_CLASS(StopBitsTest) {
	public:

		TEST_METHOD(Elements)
		{
			Assert::AreEqual(ONESTOPBIT, static_cast<int>(SimpleCom::StopBits::ONE));
			Assert::AreEqual(_T("1"), SimpleCom::StopBits::ONE.tstr());
			Assert::AreEqual(ONE5STOPBITS, static_cast<int>(SimpleCom::StopBits::ONE5));
			Assert::AreEqual(_T("1.5"), SimpleCom::StopBits::ONE5.tstr());
			Assert::AreEqual(TWOSTOPBITS, static_cast<int>(SimpleCom::StopBits::TWO));
			Assert::AreEqual(_T("2"), SimpleCom::StopBits::TWO.tstr());
		}

		TEST_METHOD(Compare)
		{
			Assert::IsTrue(SimpleCom::StopBits::ONE == SimpleCom::StopBits::ONE);
			Assert::IsFalse(SimpleCom::StopBits::ONE == SimpleCom::StopBits::ONE5);
		}

		TEST_METHOD(Values)
		{
			Assert::AreEqual(static_cast<size_t>(3), SimpleCom::FlowControl::values.size());
			Assert::IsTrue(SimpleCom::StopBits::ONE == SimpleCom::StopBits::values[0]);
			Assert::IsTrue(SimpleCom::StopBits::ONE5 == SimpleCom::StopBits::values[1]);
			Assert::IsTrue(SimpleCom::StopBits::TWO == SimpleCom::StopBits::values[2]);
		}

	};

}
