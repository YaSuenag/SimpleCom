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
#include "stdafx.h"

#include "EnumValue.h"

#define DECLARE_ENUM(cls, name, value, str) \
  const SimpleCom::cls SimpleCom::cls::name(value, str);

#define DECLARE_ENUM_VALUE(cls, name, value, str) \
  name,

#define DECLARE_ENUM_INSTANCE(cls, m) \
  m(DECLARE_ENUM) \
  const std::vector<SimpleCom::cls> SimpleCom::cls::values = { \
	m(DECLARE_ENUM_VALUE) \
  };

/*********************/

DECLARE_ENUM_INSTANCE(Parity, FOR_EACH_PARITY_ENUMS)
DECLARE_ENUM_INSTANCE(FlowControl, FOR_EACH_FLOWCTL_ENUMS)
DECLARE_ENUM_INSTANCE(StopBits, FOR_EACH_STOPBITS_ENUMS)