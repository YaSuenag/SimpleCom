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
#pragma once

#include "stdafx.h"

namespace SimpleCom {

	/*
	 * Utility class for wrinting log.
	 * THIS CLASS IS NOT THREAD SAFETY !!!
	 */
	class LogWriter
	{
	private:
		HANDLE _handle;

	public:
		LogWriter(LPCTSTR logfilename);
		virtual ~LogWriter();

		void Write(const char c);
		void Write(const char* data, const DWORD len);
	};

}