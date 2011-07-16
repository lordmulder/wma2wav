#pragma once

#include "AbstractSink.h"

class CRawWriter : public CAbstractSink
{
public:
	CRawWriter(void);
	~CRawWriter(void);
	
	virtual bool open(wchar_t *filename, WAVEFORMATEX *format);
	virtual bool write(size_t size, BYTE* buffer);
	virtual bool close(void);

private:
	FILE *m_file;
	bool m_isOpen;
};

