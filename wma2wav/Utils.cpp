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

static UINT old_cp = CP_ACP;

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
	old_cp = GetConsoleOutputCP();
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

	//setvbuf(stdout, NULL, _IONBF, 0);
	//setvbuf(stderr, NULL, _IONBF, 0);
}

void restore_previous_codepage(void)
{
	SetConsoleOutputCP(old_cp);
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
