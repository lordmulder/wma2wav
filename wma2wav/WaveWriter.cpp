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

#include "WaveWriter.h"

CWaveWriter::CWaveWriter(void)
{
	m_file = NULL;
	m_isOpen = false;
	m_dataSize = 0;
}

CWaveWriter::~CWaveWriter(void)
{
	if(m_isOpen) close();
}

bool CWaveWriter::open(wchar_t *filename, WAVEFORMATEX *format)
{
	if(m_isOpen)
	{
		return false;
	}
	
	if(_wfopen_s(&m_file, filename, L"wb"))
	{
		m_file = NULL;
		return false;
	}
	
	DWORD dwTemp = 0;
	WORD wTemp = 0;
	bool writeError = false;

	//Write the RIFF base header
	fwrite_checked("RIFF", 4, m_file, &writeError);
	dwTemp = (-1);
	fwrite_checked(&dwTemp, 4, m_file, &writeError);
	fwrite_checked("WAVE", 4, m_file, &writeError);

	//Format chunk header
	fwrite_checked("fmt ", 4, m_file, &writeError);
	dwTemp = 0x10;
	fwrite_checked(&dwTemp, 4, m_file, &writeError);
	wTemp = 0x1;
	fwrite_checked(&wTemp, 2, m_file, &writeError);
	wTemp = format->nChannels;
	fwrite_checked(&wTemp, 2, m_file, &writeError);
	dwTemp = format->nSamplesPerSec;
	fwrite_checked(&dwTemp, 4, m_file, &writeError);
	dwTemp = format->nAvgBytesPerSec;
	fwrite_checked(&dwTemp, 4, m_file, &writeError);
	wTemp = format->nBlockAlign;
	fwrite_checked(&wTemp, 2, m_file, &writeError);
	wTemp = format->wBitsPerSample;
	fwrite_checked(&wTemp, 2, m_file, &writeError);

	//Data chunk header
	fwrite_checked("data", 4, m_file, &writeError);
	dwTemp = (-1);
	fwrite_checked(&dwTemp, 4, m_file, &writeError);

	if(writeError)
	{
		fclose(m_file);
		m_file = NULL;
		return false;
	}
	
	m_dataSize = 0;
	m_isOpen = true;

	return true;
}

bool CWaveWriter::write(size_t size, BYTE* buffer)
{
	if(!m_isOpen)
	{
		return false;
	}
	
	size_t bytesWritten = fwrite(buffer, 1, size, m_file);
	m_dataSize += bytesWritten;

	return (bytesWritten == size);
}

bool CWaveWriter::close(void)
{
	if(!m_isOpen)
	{
		return false;
	}
	
	m_isOpen = false;
	
	if(_fseeki64(m_file, 0i64, SEEK_END))
	{
		fclose(m_file);
		m_file = NULL;
		return false;
	}
	
	__int64 fileSize = _ftelli64(m_file);

	if(fileSize > 0xffffffffi64)
	{
		fclose(m_file);
		m_file = NULL;
		return false;
	}

	DWORD riffSize = static_cast<DWORD>(fileSize - 8i64);
	DWORD dataSize = static_cast<DWORD>(fileSize - 44i64);

	if(dataSize != m_dataSize)
	{
		fclose(m_file);
		m_file = NULL;
		return false;
	}

	bool riffSizeUpdated = false;
	bool dataSizeUpdated = false;

	if(!fseek(m_file, 4, SEEK_SET))
	{
		if(fwrite(&riffSize, 1, 4, m_file) == 4)
		{
			riffSizeUpdated = true;
		}
	}
	if(!fseek(m_file, 40, SEEK_SET))
	{
		if(fwrite(&dataSize, 1, 4, m_file) == 4)
		{
			dataSizeUpdated = true;
		}
	}

	fclose(m_file);
	m_file = NULL;

	return (riffSizeUpdated && dataSizeUpdated);
}

void CWaveWriter::fwrite_checked(const void *data, size_t size, FILE* file, bool *errorFlag)
{
	if(fwrite(data, 1, size, file) != size)
	{
		*errorFlag = true;
	}
}
