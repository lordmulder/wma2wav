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
void seconds_to_minutes(double seconds, double *minutes_part, double *seconds_part);
size_t time_to_bytes(double time, WAVEFORMATEX *format);
double bytes_to_time(size_t bytes, WAVEFORMATEX *format);
const char *ltrim(const char *const text);
void fix_format_pcm(WAVEFORMATEX *format);

#define SAFE_DELETE(PTR) if(PTR) { delete PTR; PTR = NULL; }
#define SAFE_DELETE_ARRAY(PTR) if(PTR) { delete [] PTR; PTR = NULL; }
#define SAFE_COM_UNINIT(FLAG) if(FLAG) { CoUninitialize(); FLAG = false; }
#define CLIP3(MIN, VAL, MAX) (((VAL) > (MAX)) ? (MAX) : (((VAL) < (MIN)) ? (MIN) : (VAL)))
#define ROUND(F) (((F) >= 0.0) ? floor((F) + 0.5) : ceil((F) - 0.5))
