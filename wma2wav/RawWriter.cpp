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
