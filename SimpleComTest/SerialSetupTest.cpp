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

#include "SerialSetup.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SimpleComTest
{
	TEST_CLASS(SerialSetupTest)
	{

	public:

		TEST_METHOD(DWORDCommandLineOption) {
			SimpleCom::CommandlineOption<DWORD> opt(_T("[num]"), _T("test"), 100);

			// Default
			Assert::AreEqual(_T("[num]"), opt.GetArgs());
			Assert::AreEqual(_T("test"), opt.GetDescription());
			Assert::AreEqual(true, opt.need_arguments());
			Assert::AreEqual(static_cast<DWORD>(100), opt.get());

			// Setter
			opt.set(200);
			Assert::AreEqual(static_cast<DWORD>(200), opt.get());

			// Set from command line argument
			opt.set_from_arg(_T("300"));
			Assert::AreEqual(static_cast<DWORD>(300), opt.get());

			// Invalid command line argument
			auto test = [&] { opt.set_from_arg(_T("1a")); };
			Assert::ExpectException<std::invalid_argument>(test);
		}

		TEST_METHOD(BYTECommandLineOption) {
			SimpleCom::CommandlineOption<BYTE> opt(_T("[num]"), _T("test"), 1);

			// Default
			Assert::AreEqual(_T("[num]"), opt.GetArgs());
			Assert::AreEqual(_T("test"), opt.GetDescription());
			Assert::AreEqual(true, opt.need_arguments());
			Assert::AreEqual(static_cast<BYTE>(1), opt.get());

			// Setter
			opt.set(2);
			Assert::AreEqual(static_cast<BYTE>(2), opt.get());

			// Set from command line argument
			opt.set_from_arg(_T("3"));
			Assert::AreEqual(static_cast<BYTE>(3), opt.get());

			// Invalid command line argument
			auto test = [&] { opt.set_from_arg(_T("a")); };
			Assert::ExpectException<std::invalid_argument>(test);
		}

		TEST_METHOD(IntCommandLineOption) {
			SimpleCom::CommandlineOption<int> opt(_T("[num]"), _T("test"), 100);

			// Default
			Assert::AreEqual(_T("[num]"), opt.GetArgs());
			Assert::AreEqual(_T("test"), opt.GetDescription());
			Assert::AreEqual(true, opt.need_arguments());
			Assert::AreEqual(static_cast<int>(100), opt.get());

			// Setter
			opt.set(200);
			Assert::AreEqual(static_cast<int>(200), opt.get());

			// Set from command line argument
			opt.set_from_arg(_T("300"));
			Assert::AreEqual(static_cast<int>(300), opt.get());

			// Invalid command line argument
			auto test = [&] { opt.set_from_arg(_T("1a")); };
			Assert::ExpectException<std::invalid_argument>(test);
		}

		TEST_METHOD(BoolCommandLineOption) {
			SimpleCom::CommandlineOption<bool> opt(_T(""), _T("test"), false);

			// Default
			Assert::AreEqual(_T(""), opt.GetArgs());
			Assert::AreEqual(_T("test"), opt.GetDescription());
			Assert::AreEqual(false, opt.need_arguments());
			Assert::AreEqual(false, opt.get());

			// Setter
			opt.set(true);
			Assert::AreEqual(true, opt.get());
		}

		TEST_METHOD(DefaultValueTest)
		{
			// Default values are for Raspberry Pi
			SimpleCom::SerialSetup setup;

			Assert::AreEqual(static_cast<DWORD>(115200), setup.GetBaudRate());
			Assert::AreEqual(static_cast<BYTE>(8), setup.GetByteSize());
			Assert::AreEqual(NOPARITY, static_cast<int>(setup.GetParity()));
			Assert::AreEqual(ONESTOPBIT, static_cast<int>(setup.GetStopBits()));
			Assert::AreEqual(_T("none"), setup.GetFlowControl().tstr());
			Assert::AreEqual(false, setup.GetUseUTF8());
			Assert::AreEqual(false, setup.GetUseTTYResizer());
			Assert::AreEqual(0, setup.GetWaitDevicePeriod());
			Assert::AreEqual(false, setup.GetAutoReconnect());
			Assert::AreEqual(3, setup.GetAutoReconnectPauseInSec());
			Assert::AreEqual(120, setup.GetAutoReconnectTimeoutInSec());
		}

		TEST_METHOD(ArgParserTest)
		{
			LPCTSTR args[] = {
				_T("SimpleCom.exe"), // argv[0] is an executable in main()
				_T("--baud-rate"), _T("9600"),
				_T("--byte-size"), _T("1"),
				_T("--parity"), _T("odd"),
				_T("--stop-bits"), _T("2"),
				_T("--flow-control"), _T("hardware"),
				_T("--utf8"),
				_T("--tty-resizer"),
				_T("--show-dialog"),
				_T("--wait-serial-device"), _T("10"),
				_T("--auto-reconnect"),
				_T("--auto-reconnect-pause"), _T("5"),
				_T("--auto-reconnect-timeout"), _T("20"),
				_T("COM100")
			};

			SimpleCom::SerialSetup setup;
			setup.ParseArguments(sizeof(args) / sizeof(char*), args);

			Assert::AreEqual(static_cast<DWORD>(9600), setup.GetBaudRate());
			Assert::AreEqual(static_cast<BYTE>(1), setup.GetByteSize());
			Assert::AreEqual(ODDPARITY, static_cast<int>(setup.GetParity()));
			Assert::AreEqual(TWOSTOPBITS, static_cast<int>(setup.GetStopBits()));
			Assert::AreEqual(_T("hardware"), setup.GetFlowControl().tstr());
			Assert::AreEqual(true, setup.GetUseUTF8());
			Assert::AreEqual(true, setup.GetUseTTYResizer());
			Assert::AreEqual(10, setup.GetWaitDevicePeriod());
			Assert::AreEqual(true, setup.GetAutoReconnect());
			Assert::AreEqual(5, setup.GetAutoReconnectPauseInSec());
			Assert::AreEqual(20, setup.GetAutoReconnectTimeoutInSec());
			Assert::AreEqual(_T("COM100"), setup.GetPort().c_str());
		}

		TEST_METHOD(SaveToDCBTestForDefault)
		{
			DCB dcb;
			SimpleCom::SerialSetup setup;

			// default values
			setup.SaveToDCB(&dcb);
			Assert::AreEqual(static_cast<DWORD>(sizeof(DCB)), dcb.DCBlength);
			Assert::AreEqual(setup.GetBaudRate(), dcb.BaudRate);
			Assert::AreEqual(static_cast<DWORD>(TRUE), dcb.fBinary);
			Assert::AreEqual(static_cast<DWORD>(setup.GetParity() != NOPARITY), dcb.fParity);
			Assert::AreEqual(static_cast<BYTE>(NOPARITY), dcb.Parity);
			Assert::AreEqual(setup.GetByteSize(), dcb.ByteSize);
			Assert::AreEqual(static_cast<BYTE>(setup.GetStopBits()), dcb.StopBits);
		}

		TEST_METHOD(SaveToDCBTestForNoParity)
		{
			DCB dcb;
			SimpleCom::SerialSetup setup;

			// not NOPARITY
			LPCTSTR args[] = {
				_T("SimpleCom.exe"), // argv[0] is an executable in main()
				_T("--parity"), _T("odd"),
				_T("COM100")
			};
			setup.ParseArguments(sizeof(args) / sizeof(char*), args);
			setup.SaveToDCB(&dcb);
			Assert::AreEqual(static_cast<DWORD>(setup.GetParity() != NOPARITY), dcb.fParity);
			Assert::AreEqual(static_cast<BYTE>(ODDPARITY), dcb.Parity);
		}

		TEST_METHOD(SaveToDCBTestForHardFlowCtl)
		{
			DCB dcb;
			SimpleCom::SerialSetup setup;

			// Hardware flow control
			LPCTSTR args[] = {
				_T("SimpleCom.exe"), // argv[0] is an executable in main()
				_T("--flow-control"), _T("hardware"),
				_T("COM100")
			};
			setup.ParseArguments(sizeof(args) / sizeof(char*), args);
			setup.SaveToDCB(&dcb);
			Assert::AreEqual(static_cast<DWORD>(TRUE), dcb.fOutxCtsFlow);
			Assert::AreEqual(static_cast<DWORD>(RTS_CONTROL_HANDSHAKE), dcb.fRtsControl);
		}

		TEST_METHOD(SaveToDCBTestForSoftFlowCtl)
		{
			DCB dcb;
			SimpleCom::SerialSetup setup;

			// Software flow control
			LPCTSTR args[] = {
				_T("SimpleCom.exe"), // argv[0] is an executable in main()
				_T("--flow-control"), _T("software"),
				_T("COM100")
			};
			setup.ParseArguments(sizeof(args) / sizeof(char*), args);
			setup.SaveToDCB(&dcb);
			Assert::AreEqual(static_cast<DWORD>(TRUE), dcb.fOutX);
			Assert::AreEqual(static_cast<DWORD>(TRUE), dcb.fInX);
			Assert::AreEqual(static_cast<WORD>(2048), dcb.XonLim);
			Assert::AreEqual(static_cast<WORD>(2048), dcb.XoffLim);
			Assert::AreEqual('\x11', dcb.XonChar);
			Assert::AreEqual('\x13', dcb.XoffChar);
		}

	};
}
