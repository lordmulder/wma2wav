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

char *utf16_to_utf8(const wchar_t *input);
wchar_t *utf8_to_utf16(const char *input);
void repair_standard_streams(void);
void restore_previous_codepage(void);
bool safe_com_init(void);
bool safe_com_uninit(void);
void seconds_to_minutes(double seconds, double *minutes_part, double *seconds_part);
size_t time_to_bytes(double time, WAVEFORMATEX *format);
double bytes_to_time(size_t bytes, WAVEFORMATEX *format);
const char *ltrim(const char *const text);
void fix_format_pcm(WAVEFORMATEX *format);
void set_console_color(FILE* file, WORD attributes);
void restore_console_color(FILE* file);
bool secure_load_library(HMODULE *module, const wchar_t* fileName);

#define SAFE_DELETE(PTR) if(PTR) { delete PTR; PTR = NULL; }
#define SAFE_DELETE_ARRAY(PTR) if(PTR) { delete [] PTR; PTR = NULL; }
#define CLIP3(MIN, VAL, MAX) (((VAL) > (MAX)) ? (MAX) : (((VAL) < (MIN)) ? (MIN) : (VAL)))
#define LIMIT_TO(VAL, MAX) VAL = min((VAL), (MAX))
#define ROUND(F) (((F) >= 0.0) ? floor((F) + 0.5) : ceil((F) - 0.5))
#define VALID_HANDLE(H) (((H) != NULL) && ((H) != INVALID_HANDLE_VALUE))
#define STREQ(STR1, STR2) (_wcsicmp(STR1, STR2) == 0)

#if defined(__INTEL_COMPILER)
#if (__INTEL_COMPILER >= 1200)
#define __COMPILER__ "ICL 12.x"
#elif (__INTEL_COMPILER >= 1100)
#define __COMPILER__ "ICL 11.x"
#elif (__INTEL_COMPILER >= 1000)
#define __COMPILER__ "ICL 10.x"
#else
#error Compiler is not supported!
#endif
#elif defined(_MSC_VER)
#if (_MSC_VER == 1600)
#if (_MSC_FULL_VER >= 160040219)
#define __COMPILER__ "MSVC 10.0-SP1"
#else
#define __COMPILER__ "MSVC 10.0"
#endif
#elif (_MSC_VER == 1500)
#if (_MSC_FULL_VER >= 150030729)
#define __COMPILER__ "MSVC 9.0-SP1"
#else
#define __COMPILER__ "MSVC 9.0"
#endif
#else
#error Compiler is not supported!
#endif
#else
#error Compiler is not supported!
#endif

#if defined(_M_X64)
#define __ARCH__ "x64"
#else
#define __ARCH__ "x86"
#endif

#ifdef _DEBUG
#define PING cerr << "\n\nPING: " << __FILE__ << " @ " << __LINE__ << "\n" << endl
int dbg_printf(wchar_t *format, ...);
#else
#define PING
#define dbg_printf(FMT, ...)
#endif
