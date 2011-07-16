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

#include "utils.h"
#include <io.h>

char *utf16_to_utf8(const wchar_t *input)
{
	char *Buffer;
	int BuffSize, Result;

	BuffSize = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
	Buffer = new char[BuffSize]; //(char*) malloc(sizeof(char) * BuffSize);
	Result = WideCharToMultiByte(CP_UTF8, 0, input, -1, Buffer, BuffSize, NULL, NULL);

	return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
}

wchar_t *utf8_to_utf16(const char *input)
{
	wchar_t *Buffer;
	int BuffSize, Result;

	BuffSize = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
	Buffer = new wchar_t[BuffSize]; //(wchar_t*) malloc(sizeof(wchar_t) * BuffSize);
	Result = MultiByteToWideChar(CP_UTF8, 0, input, -1, Buffer, BuffSize);

	return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
}

void repair_standard_streams(void)
{
	SetConsoleOutputCP(CP_UTF8);

	int hCrtStdOut = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), 0);
	int hCrtStdErr = _open_osfhandle((long) GetStdHandle(STD_ERROR_HANDLE), 0);

	if(hCrtStdOut >= 0)
	{
		FILE *hfStdout = _fdopen(hCrtStdOut, "w");
		if(hfStdout) *stdout = *hfStdout;
	}

	if(hCrtStdErr >= 0)
	{
		FILE *hfStderr = _fdopen(hCrtStdErr, "w");
		if(hfStderr) *stderr = *hfStderr;
	}
}
