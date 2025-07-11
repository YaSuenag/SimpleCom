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
#pragma once

#include "stdafx.h"
#include "TerminalRedirectorBase.h"
#include "util.h"

namespace SimpleCom
{
    typedef struct {
        HANDLE hSerial;
        HANDLE hStdOut;
        OVERLAPPED overlapped;
        SimpleCom::LogWriter* logwriter;
        HANDLE hTermEvent;
        std::function<void(const SimpleCom::WinAPIException&)> exception_handler;
    } TStdOutRedirectorParam;

    typedef struct {
        HANDLE hSerial;
        HANDLE hStdIn;
        HANDLE hStdOut;
        bool enableStdinLogging;
        SimpleCom::LogWriter* logwriter;
        bool useTTYResizer;
        HWND parent_hwnd;
        HANDLE hTermEvent;
        std::function<void(const SimpleCom::WinAPIException&)> exception_handler;
        bool* reattachable;
    } TStdInRedirectorParam;

    class TerminalRedirector :
        public TerminalRedirectorBase
    {
    private:
        HandleHandler _hTermEvent;
        HandleHandler _hIoEvent;
        concurrency::concurrent_queue<WinAPIException> _exception_queue;
        bool _reattachable;
        TStdInRedirectorParam _stdin_param;
        TStdOutRedirectorParam _stdout_param;

    protected:
        virtual std::tuple<LPTHREAD_START_ROUTINE, LPVOID> GetStdInRedirector() override;
        virtual std::tuple<LPTHREAD_START_ROUTINE, LPVOID> GetStdOutRedirector() override;

    public:
        TerminalRedirector(HANDLE hSerial, LogWriter* logwriter, bool enableStdinLogging, bool useTTYResizer, HWND parent_hwnd);
        virtual ~TerminalRedirector() {};

        inline concurrency::concurrent_queue<WinAPIException> & exception_queue() {
            return _exception_queue;
		}

        void StartRedirector() override;
        bool Reattachable() override;
    };
}