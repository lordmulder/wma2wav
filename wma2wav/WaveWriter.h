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

#include "AbstractSink.h"
#include "stdafx.h"

class CWaveWriter :	public CAbstractSink
{
public:
	CWaveWriter(void);
	~CWaveWriter(void);

	virtual bool open(wchar_t *filename, WAVEFORMATEX *format);
	virtual bool write(size_t size, BYTE* buffer);
	virtual bool close(void);

private:
	FILE* m_file;
	bool m_isOpen;
	unsigned __int64 m_dataSize;

	static void fwrite_checked(const void *data, size_t size, FILE* file, bool *errorFlag);
};

