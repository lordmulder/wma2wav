#pragma once

#include "stdafx.h"

class CAbstractSink
{
public:
	CAbstractSink(void);
	~CAbstractSink(void);

	virtual bool open(wchar_t *filename, WAVEFORMATEX *format) = 0;
	virtual bool write(size_t size, BYTE* buffer) = 0;
	virtual bool close(void) = 0;
};
