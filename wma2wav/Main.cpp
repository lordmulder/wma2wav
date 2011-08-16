///////////////////////////////////////////////////////////////////////////////
// wma2wav - Dump WMA files to Wave Audio
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "wma2wav.h"
#include "Utils.h"

static void invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
	fprintf(stderr, "\nInternal Error: Invalid parameters handler was invoked.\n");
	fprintf(stderr, "This incident should be reported to the developer!\n");
	TerminateProcess(GetCurrentProcess(), -1);
}

static LONG WINAPI unhandled_exception_filter(struct _EXCEPTION_POINTERS *dummy)
{
	fprintf(stderr, "\nInternal Error: Unhandled exception filter was invoked.\n");
	fprintf(stderr, "This incident should be reported to the developer!\n");
	TerminateProcess(GetCurrentProcess(), -1);
	return -1;
}

static BOOL WINAPI ctrl_handler_routine(DWORD dwCtrlType)
{
	g_aborted = true;
	return TRUE;
}

static int wmain2(int argc, _TCHAR* argv[])
{
	try
	{
		return wma2wav(argc, argv);
	}
	catch(std::bad_alloc err)
	{
		fprintf(stderr, "\nMemory allocation has failed, application will exit!\n");
		return -1;
	}
	catch(char *err)
	{
		fprintf(stderr, "\n%s\n", err);
		return -1;
	}
	catch(...)
	{
		fprintf(stderr, "\nUnhandeled exception error, application will exit!\n");
		return -1;
	}
}

int wmain(int argc, _TCHAR* argv[])
{
	_set_invalid_parameter_handler(invalid_param_handler);
	SetUnhandledExceptionFilter(unhandled_exception_filter);
	SetConsoleCtrlHandler(ctrl_handler_routine, TRUE);

	__try
	{
		repair_standard_streams();
		int result = wmain2(argc, argv);
		safe_com_uninit();
		restore_previous_codepage();
		return result;
	}
	__except(1)
	{
		fprintf(stderr, "\nUnhandeled system exception, application will exit!\n");
		TerminateProcess(GetCurrentProcess(), -1);
		return -1;
	}
}
