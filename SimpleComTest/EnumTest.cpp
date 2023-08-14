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
