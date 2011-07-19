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

#include "RawWriter.h"

CRawWriter::CRawWriter(void)
{
	m_file = NULL;
	m_isOpen = false;
}

CRawWriter::~CRawWriter(void)
{
	if(m_isOpen) close();
}

bool CRawWriter::open(wchar_t *filename, WAVEFORMATEX *format)
{
	if(m_isOpen)
	{
		return false;
	}
	
	if(_wcsicmp(filename, L"-"))
	{
		if(_wfopen_s(&m_file, filename, L"wb"))
		{
			m_file = NULL;
			return false;
		}
	}
	else
	{
		m_file = stdout;
	}
	
	m_isOpen = true;
	return true;
}

bool CRawWriter::write(size_t size, BYTE* buffer)
{
	if(!m_isOpen)
	{
		return false;
	}
	
	size_t bytesWritten = fwrite(buffer, 1, size, m_file);
	return (bytesWritten == size);
}

bool CRawWriter::close(void)
{
	if(!m_isOpen)
	{
		return false;
	}
	
	if(m_file != stdout)
	{
		fclose(m_file);
		m_file = NULL;
	}

	m_isOpen = false;
	return true;
}
