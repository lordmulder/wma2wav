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
#include <math.h>
#include <map>
#include <Objbase.h>

using namespace std;

typedef BOOL (__stdcall *SetDllDirectoryProc)(LPCWSTR lpPathName);

static RTL_CRITICAL_SECTION g_lock;
static int init_lock(RTL_CRITICAL_SECTION *lock);
static int g_lock_init_done = init_lock(&g_lock);
static bool g_com_initialized = false;
static map<FILE*,WORD> g_old_text_attrib;
static UINT g_old_cp = CP_ACP;

char *utf16_to_utf8(const wchar_t *input)
{
	char *Buffer;
	int BuffSize, Result;

	BuffSize = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
	
	if(BuffSize > 0)
	{
		Buffer = new char[BuffSize]; //(char*) malloc(sizeof(char) * BuffSize);
		Result = WideCharToMultiByte(CP_UTF8, 0, input, -1, Buffer, BuffSize, NULL, NULL);
		return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
	}

	return NULL;
}

wchar_t *utf8_to_utf16(const char *input)
{
	wchar_t *Buffer;
	int BuffSize, Result;

	BuffSize = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);

	if(BuffSize > 0)
	{
		Buffer = new wchar_t[BuffSize]; //(wchar_t*) malloc(sizeof(wchar_t) * BuffSize);
		Result = MultiByteToWideChar(CP_UTF8, 0, input, -1, Buffer, BuffSize);
		return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
	}

	return NULL;
}

void repair_standard_streams(void)
{
	EnterCriticalSection(&g_lock);

	__try
	{
		g_old_cp = GetConsoleOutputCP();
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
	__finally
	{
		LeaveCriticalSection(&g_lock);
	}
}

void restore_previous_codepage(void)
{
	EnterCriticalSection(&g_lock);

	__try
	{
		SetConsoleOutputCP(g_old_cp);
	}
	__finally
	{
		LeaveCriticalSection(&g_lock);
	}
}

bool safe_com_init(void)
{
	bool success = false;
	EnterCriticalSection(&g_lock);

	if(g_com_initialized)
	{
		LeaveCriticalSection(&g_lock);
		return false;
	}

	__try
	{
		if(CoInitializeEx(NULL, COINIT_MULTITHREADED) == S_OK)
		{
			g_com_initialized = true;
			success = true;
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_lock);
	}

	return success;
}

bool safe_com_uninit(void)
{
	EnterCriticalSection(&g_lock);

	if(!(g_com_initialized))
	{
		LeaveCriticalSection(&g_lock);
		return false;
	}

	__try
	{
		CoUninitialize();
		g_com_initialized = false;
	}
	__finally
	{
		LeaveCriticalSection(&g_lock);
	}

	return true;
}

void seconds_to_minutes(double seconds, double *minutes_part, double *seconds_part)
{
	double _minutes = 0.0;
	double _seconds = modf((seconds / 60.0), &_minutes);
	*minutes_part = _minutes;
	*seconds_part = _seconds * 60.0;
}

size_t time_to_bytes(double time, WAVEFORMATEX *format)
{
	return static_cast<size_t>(ROUND(time * static_cast<double>(format->nSamplesPerSec))) * (format->wBitsPerSample / 8) * format->nChannels;
}

double bytes_to_time(size_t bytes, WAVEFORMATEX *format)
{
	return static_cast<double>(bytes / (format->wBitsPerSample / 8) / format->nChannels) / static_cast<double>(format->nSamplesPerSec);
}

const char *ltrim(const char *const text)
{
	const char *ptr = text;
	while(ptr[0] == 0x20) ptr++;
	return ptr;
}

void fix_format_pcm(WAVEFORMATEX *format)
{
	format->wFormatTag = WAVE_FORMAT_PCM;
	format->nChannels = CLIP3(1, format->nChannels, 8);
	format->nSamplesPerSec = CLIP3(8000, format->nSamplesPerSec, 192000);
	format->wBitsPerSample = CLIP3(1, (format->wBitsPerSample / 8), 3) * 8;
	format->nBlockAlign = (format->wBitsPerSample * format->nChannels) / 8;
	format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
}

void set_console_color(FILE* file, WORD attributes)
{
	EnterCriticalSection(&g_lock);

	try
	{
		const HANDLE hConsole = (HANDLE)(_get_osfhandle(_fileno(file)));
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		if(GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
		{
			g_old_text_attrib.insert(pair<FILE*,WORD>(file, consoleInfo.wAttributes));
		}
		SetConsoleTextAttribute(hConsole, attributes);
	}
	catch(...)
	{
		dbg_printf(L"set_console_color: Failed to set console attributes (exception error)");
	}
	
	LeaveCriticalSection(&g_lock);
}

void restore_console_color(FILE* file)
{
	EnterCriticalSection(&g_lock);

	try
	{
		if(g_old_text_attrib.find(file) != g_old_text_attrib.end())
		{
			const HANDLE hConsole = (HANDLE)(_get_osfhandle(_fileno(file)));
			SetConsoleTextAttribute(hConsole, g_old_text_attrib[file]);
		}
	}
	catch(...)
	{
		dbg_printf(L"set_console_color: Failed to restore console attributes (exception error)");
	}

	LeaveCriticalSection(&g_lock);
}

bool secure_load_library(HMODULE *module, const wchar_t* fileName)
{
	*module = NULL;
	bool success = false;
	
	EnterCriticalSection(&g_lock);

	__try
	{
		UINT oldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
		HMODULE hKernel = LoadLibraryW(L"kernel32.dll");

		if(VALID_HANDLE(hKernel))
		{
			SetDllDirectoryProc pSetDllDirectory = reinterpret_cast<SetDllDirectoryProc>(GetProcAddress(hKernel, "SetDllDirectoryW"));
			if(pSetDllDirectory != NULL) pSetDllDirectory(L"");
			FreeLibrary(hKernel);
			hKernel = NULL;
		}

		HMODULE temp = LoadLibraryW(fileName);
	
		if(VALID_HANDLE(temp))
		{
			*module = temp;
			success = true;
		}

		SetErrorMode(oldErrorMode);
	}
	__finally
	{
		LeaveCriticalSection(&g_lock);
	}
	
	return success;
}

#ifdef _DEBUG
size_t _dbg_printf(wchar_t *format, ...)
{
	size_t len = 0;
	va_list args;
	va_start (args, format);
	if(format)
	{
		wchar_t buffer[1024];
		len = _vsnwprintf_s(buffer, 1024, _TRUNCATE, format, args);
		if(len > 0) OutputDebugStringW(buffer);
	}
	else
	{
		wchar_t *str = va_arg(args, wchar_t*);
		len = wcslen(str);
		if(len > 0) OutputDebugStringW(str);
	}
	va_end (args);
	return len;
}
#endif

static int init_lock(RTL_CRITICAL_SECTION *lock)
{
	InitializeCriticalSection(lock);
	return TRUE;
}
