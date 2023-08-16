#include "pch.h"
#include "CppUnitTest.h"

#include "SerialPortWriter.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SimpleComTest
{
	TEST_CLASS(SerialPortWriterTest)
	{
	private:
		HANDLE hWrite;
		HANDLE hRead;

		DWORD bytes_available_in_pipe() {
			DWORD bytes_avail;

			if (!PeekNamedPipe(hRead, NULL, 0, NULL, &bytes_avail, NULL)) {
				throw _T("PeekNamedPipe()");
			}

			return bytes_avail;
		}

	public:

		TEST_METHOD_INITIALIZE(Initialize) {
			if (!CreatePipe(&hRead, &hWrite, NULL, 10)) {
				Assert::Fail(_T("CreatePipe() failed"));
			}
		}

		TEST_METHOD_CLEANUP(Cleanup) {
			CloseHandle(hRead);
			CloseHandle(hWrite);
		}

		TEST_METHOD(WriteAsyncTest)
		{
			SimpleCom::SerialPortWriter writer(hWrite, 3);

			// Pipe should be empty
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());

			// Pipe should be still empty
			writer.WriteAsync();
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());

			// Pipe should be still empty because data is not flushed
			writer.Put('1');
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());

			// '1' should be pulled from pipe
			writer.WriteAsync();
			char ch;
			if (!ReadFile(hRead, &ch, 1, NULL, NULL)) {
				throw _T("ReadFile()");
			}
			Assert::AreEqual('1', ch);

			// Pipe should be empty
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());
		}

		TEST_METHOD(PutTest)
		{
			SimpleCom::SerialPortWriter writer(hWrite, 3);

			// Pipe should be still empty because data is not flushed
			writer.Put('1');
			writer.Put('2');
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());

			// '1' - '3' should be read from pipe because the buffer is flushed automatically
			writer.Put('3');
			char chars[3];
			if (!ReadFile(hRead, &chars, sizeof(chars), NULL, NULL)) {
				throw _T("ReadFile()");
			}
			Assert::AreEqual('1', chars[0]);
			Assert::AreEqual('2', chars[1]);
			Assert::AreEqual('3', chars[2]);

			// Pipe should be empty
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());
		}

		TEST_METHOD(PutDataTest)
		{
			SimpleCom::SerialPortWriter writer(hWrite, 3);

			// All data should be read from pipe
			writer.Put('1');
			writer.PutData("abc", 4);
			char chars[5];
			if (!ReadFile(hRead, &chars, sizeof(chars), NULL, NULL)) {
				throw _T("ReadFile()");
			}
			Assert::AreEqual("1abc", chars);

			// Pipe should be empty
			writer.WriteAsync();
			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());
		}

		TEST_METHOD(ShutdownTest)
		{
			// All data should be discarded from pipe when the writer is shutdown
			{
				SimpleCom::SerialPortWriter writer(hWrite, 3);

				writer.Put('1');
				writer.Shutdown();
				Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());
			}

			Assert::AreEqual(static_cast<DWORD>(0), bytes_available_in_pipe());
		}

	};
}
