/*
 * Copyright (C) 2019, 2023, Yasumasa Suenaga
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
#pragma once

#include "stdafx.h"

#define FOR_EACH_PARITY_ENUMS(f) \
  f(Parity, NO_PARITY,    NOPARITY,    _T("none"))  \
  f(Parity, ODD_PARITY,   ODDPARITY,   _T("odd"))   \
  f(Parity, EVEN_PARITY,  EVENPARITY,  _T("even"))  \
  f(Parity, MARK_PARITY,  MARKPARITY,  _T("mark"))  \
  f(Parity, SPACE_PARITY, SPACEPARITY, _T("space"))

#define FOR_EACH_FLOWCTL_ENUMS(f) \
  f(FlowControl, NONE,     0, _T("none"))     \
  f(FlowControl, HARDWARE, 1, _T("hardware")) \
  f(FlowControl, SOFTWARE, 2, _T("software"))

#define FOR_EACH_STOPBITS_ENUMS(f) \
  f(StopBits, ONE,  ONESTOPBIT,   _T("1"))   \
  f(StopBits, ONE5, ONE5STOPBITS, _T("1.5")) \
  f(StopBits, TWO,  TWOSTOPBITS,  _T("2"))


namespace SimpleCom {

	/*
	 * Base class for enum value.
	 * This class holds int value (like enum type) and description in string.
	 * The instance would be compared in int value.
	 */
	class EnumValue
	{
	protected:
		int _value;
		LPTSTR _str;

	public:
		constexpr explicit EnumValue(const int value, LPCTSTR str) noexcept : _value(value), _str(const_cast<LPTSTR>(str)) {};

		constexpr operator int() const noexcept { return _value; };
		bool operator == (EnumValue& rvalue) { return _value == rvalue._value; };

		inline constexpr LPCTSTR tstr() const noexcept { return _str; };
	};

	/* Enum for parity */
	class Parity : public EnumValue {
	public:
		constexpr explicit Parity(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};

#define DECLARE_ENUM(cls, name, value, str) \
		static const cls name;

		FOR_EACH_PARITY_ENUMS(DECLARE_ENUM)
#undef DECLARE_ENUM

		static const std::vector<Parity> values;

		inline static TString valueopts() {
			TString str = _T("[");
			for (auto& value : values) {
				str += value.tstr();
				str += _T('|');
			}
			str[str.length() - 1] = _T(']');

			return str;
		}

	};

	/* Enum for flow control */
	class FlowControl : public EnumValue {
	public:
		constexpr explicit FlowControl(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};

#define DECLARE_ENUM(cls, name, value, str) \
		static const cls name;

		FOR_EACH_FLOWCTL_ENUMS(DECLARE_ENUM)
#undef DECLARE_ENUM

		static const std::vector<FlowControl> values;

		inline static TString valueopts() {
			TString str = _T("[");
			for (auto& value : values) {
				str += value.tstr();
				str += _T('|');
			}
			str[str.length() - 1] = _T(']');

			return str;
		}

	};

	/* Enum for stop bits */
	class StopBits : public EnumValue {
	public:
		constexpr explicit StopBits(const int value, LPCTSTR str) noexcept : EnumValue(value, str) {};

#define DECLARE_ENUM(cls, name, value, str) \
		static const cls name;

		FOR_EACH_STOPBITS_ENUMS(DECLARE_ENUM)
#undef DECLARE_ENUM

		static const std::vector<StopBits> values;

		inline static TString valueopts() {
			TString str = _T("[");
			for (auto& value : values) {
				str += value.tstr();
				str += _T('|');
			}
			str[str.length() - 1] = _T(']');

			return str;
		}

	};

}