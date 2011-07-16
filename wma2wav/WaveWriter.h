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

