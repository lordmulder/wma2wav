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

#pragma once

#include "stdafx.h"
struct IWMSyncReader;

class CWmaReader
{
public:
	CWmaReader(void);
	~CWmaReader(void);

	bool open(const wchar_t *filename);
	void close(void);
	bool analyze(void);
	bool getFormat(WAVEFORMATEX *format);
	size_t getSampleSize(void);
	double getDuration(void);
	bool getNextSample(BYTE *output, size_t *length);

private:
	HMODULE m_wmvCore;
	IWMSyncReader *m_reader;
	WAVEFORMATEX *m_format;
	DWORD m_outputNum;
	WORD m_streamNum;
	
	bool m_isOpen;
	bool m_isAnalyzed;

};

