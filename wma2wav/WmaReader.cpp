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

#include "WmaReader.h"
#include <Wmsdk.h>

using namespace std;

typedef HRESULT (__stdcall *WMCreateSyncReaderProc)(IUnknown* pUnkCert, DWORD dwRights, IWMSyncReader **ppSyncReader);

#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800

CWmaReader::CWmaReader(void)
{
	m_isOpen = false;
	m_isAnalyzed = false;
	m_wmvCore = NULL;
	m_reader = NULL;
	m_format = NULL;
	m_outputNum = -1;
	m_streamNum = -1;

	HMODULE m_wmvCore = LoadLibraryExW(L"wmvcore.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!(m_wmvCore != NULL))
	{
		throw "Fatal Error: Failed to load WMVCORE.DLL libraray!";
	}
	
	WMCreateSyncReaderProc pWMCreateSyncReader = reinterpret_cast<WMCreateSyncReaderProc>(GetProcAddress(m_wmvCore, "WMCreateSyncReader"));	
	if(!(pWMCreateSyncReader != NULL))
	{
		throw "Fatal Error: Entry point 'WMCreateSyncReader' not be found!";
	}
	
	if(pWMCreateSyncReader(NULL, 0, &m_reader) != S_OK)
	{
		m_reader = NULL;
		throw "Fatal Error: Failed to create IWMSyncReader interface!";
	}
}

CWmaReader::~CWmaReader(void)
{
	if(m_reader)
	{
		m_reader->Release();
		m_reader = NULL;
	}
	if(m_format)
	{
		delete m_format;
		m_format = NULL;
	}
	if(m_wmvCore)
	{
		FreeLibrary(m_wmvCore);
		m_wmvCore = NULL;
	}
}

bool CWmaReader::open(const wchar_t *filename)
{
	if(m_isOpen)
	{
		return false;
	}

	if(m_reader->Open(filename) == S_OK)
	{
		m_isOpen = true;
		return true;
	}

	return false;
}

void CWmaReader::close(void)
{
	if(m_isOpen)
	{
		m_reader->Close();
		m_isOpen = false;
	}
}

bool CWmaReader::analyze(void)
{
	if((!m_isOpen) || m_isAnalyzed)
	{
		return false;
	}

	DWORD outputCount = 0;

	if(m_reader->GetOutputCount(&outputCount) != S_OK)
	{
		return false;
	}

	bool foundAudioStream = false;

	for(DWORD i = 0; i < outputCount; i++)
	{
		if(foundAudioStream)
		{
			break;
		}
		
		IWMOutputMediaProps *props = NULL;
		
		if(m_reader->GetOutputProps(i, &props) == S_OK)
		{
			DWORD size = 0;

			if(props->GetMediaType(NULL, &size) == S_OK)
			{
				char *buffer =  new char[size];
				WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);
				
				if(props->GetMediaType(mediaType, &size) == S_OK)
				{
					if(mediaType->formattype == WMFORMAT_WaveFormatEx)
					{
						WORD streamNum = -1;
						
						if(m_reader->GetStreamNumberForOutput(i, &streamNum) == S_OK)
						{
							if(m_reader->SetReadStreamSamples(streamNum, FALSE) == S_OK)
							{
								BOOL isCompressed = TRUE;
							
								if(m_reader->GetReadStreamSamples(streamNum, &isCompressed) == S_OK)
								{
									if(isCompressed == FALSE)
									{
										m_format = new WAVEFORMATEX;
										memcpy(m_format, mediaType->pbFormat, sizeof(WAVEFORMATEX));
										m_outputNum = i;
										m_streamNum = streamNum;
										foundAudioStream = true;
									}
								}
							}
						}
					}
				}
				
				delete [] buffer;
			}

			props->Release();
			props = NULL;
		}
	}

	if(foundAudioStream)
	{
		m_isAnalyzed = true;
	}

	return foundAudioStream;
}

double CWmaReader::getDuration(void)
{
	double duration = -1.0;

	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}
	
	IWMHeaderInfo* pHdrInfo = NULL;
	
	if(m_reader->QueryInterface(IID_IWMHeaderInfo,(void**)&pHdrInfo) == S_OK)
	{
		WMT_ATTR_DATATYPE dType;
		WORD size = 0;
		WORD stream = 0; //m_streamNum

		if(pHdrInfo->GetAttributeByName(&stream, L"Duration", &dType, NULL, &size) == S_OK)
		{
			if((dType == WMT_TYPE_QWORD) && (size == sizeof(QWORD)))
			{
				BYTE pValue[sizeof(QWORD)];

				if(pHdrInfo->GetAttributeByName(&stream, L"Duration", &dType, (BYTE*)&pValue, &size) == S_OK)
				{
					duration = static_cast<double>((*reinterpret_cast<QWORD*>(pValue)) / 1000) / 10000.0;
				}
			}
		}
		
		pHdrInfo->Release();
	}

	return duration;
}

bool CWmaReader::getFormat(WAVEFORMATEX *format)
{
	SecureZeroMemory(format, sizeof(WAVEFORMATEX));
	
	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}

	memcpy(format, m_format, sizeof(WAVEFORMATEX));
	return true;
}

size_t CWmaReader::getSampleSize(void)
{
	DWORD streamMax = 0;
	DWORD outputMax = 0;
	
	if(m_reader->GetMaxOutputSampleSize(m_outputNum, &outputMax) == S_OK)
	{
		if(m_reader->GetMaxStreamSampleSize(m_streamNum, &streamMax) == S_OK)
		{
			return max(outputMax, streamMax);
		}
	}
	
	return 0;
}

bool CWmaReader::getNextSample(BYTE *output, size_t *length, double *timeStamp, double *sampleDuration)
{
	*length = 0;
	if(timeStamp) *timeStamp = -1.0;
	if(sampleDuration) *sampleDuration = -1.0;

	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}

	INSSBuffer *buffer = 0;
	QWORD time = 0;
	QWORD duration = 0;
	DWORD flags = 0;
	DWORD sampleOutputNo = 0;
	WORD sampleStreamNo = 0;
	DWORD bufferLen = 0;
	BYTE *bufferPtr;
	HRESULT result = 0;

	if((result = m_reader->GetNextSample(m_streamNum, &buffer, &time, &duration, &flags, &sampleOutputNo, &sampleStreamNo)) != S_OK)
	{
		return (result == NS_E_NO_MORE_SAMPLES) ? true : false;
	}

	if(buffer->GetLength(&bufferLen) != S_OK)
	{
		buffer->Release();
		return false;
	}

	if(buffer->GetBuffer(&bufferPtr) != S_OK)
	{
		buffer->Release();
		return false;
	}

	memcpy(output, bufferPtr, bufferLen);
	*length = bufferLen;
	
	if(timeStamp) *timeStamp = static_cast<double>(time / 1000) / 10000.0;
	if(sampleDuration) *sampleDuration = static_cast<double>(duration / 1000) / 10000.0;
	
	buffer->Release();
	return true;
}
