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
#include "Utils.h"
#include <Wmsdk.h>

using namespace std;

typedef BOOL (__stdcall *SetDllDirectoryProc)(LPCWSTR lpPathName);
typedef HRESULT (__stdcall *WMCreateSyncReaderProc)(IUnknown* pUnkCert, DWORD dwRights, IWMSyncReader **ppSyncReader);
typedef HRESULT (__stdcall *WMIsContentProtectedProc)(const WCHAR *pwszFileName, BOOL *pfIsProtected);
typedef HRESULT (__stdcall *WMValidateDataProc)(BYTE *pbData, DWORD *pdwDataSize);

#define NANOTIME_TO_DOUBLE(T) (static_cast<double>((T) / 1000) / 10000.0)
#define VALID_HANDLE(H) (((H) != NULL) && ((H) != INVALID_HANDLE_VALUE))

CWmaReader::CWmaReader(void)
{
	m_isOpen = false;
	m_isAnalyzed = false;
	m_wmvCore = NULL;
	m_wmvCoreVersion[0] = 0;
	m_wmvCoreVersion[1] = 0;
	m_wmvCoreVersion[2] = 0;
	m_wmvCoreVersion[3] = 0;
	m_reader = NULL;
	m_outputNum = -1;
	m_streamNum = -1;
	
	HMODULE hKernel = LoadLibraryW(L"kernel32.dll");

	if(VALID_HANDLE(hKernel))
	{
		SetDllDirectoryProc pSetDllDirectory = reinterpret_cast<SetDllDirectoryProc>(GetProcAddress(hKernel, "SetDllDirectoryW"));
		if(pSetDllDirectory)
		{
			pSetDllDirectory(L"");
		}
		FreeLibrary(hKernel);
		hKernel = NULL;
	}

	m_wmvCore = LoadLibraryW(L"wmvcore.dll");

	if(!(VALID_HANDLE(m_wmvCore)))
	{
		throw "Fatal Error: Failed to load WMVCORE.DLL libraray!\nWindows Media Format Runtime (Version 9+) is required.";
	}
	
	wchar_t wmvCorePath[1024];

	if(GetModuleFileNameW(m_wmvCore, wmvCorePath, 1024))
	{
		wmvCorePath[1023] = L'\0';
		DWORD verInfoSize = GetFileVersionInfoSize(wmvCorePath, NULL);
		BYTE *verInfo = new BYTE[verInfoSize];

		if(GetFileVersionInfo(wmvCorePath, NULL, verInfoSize, verInfo))
		{
			UINT fixedVerInfoSize = 0;
			VS_FIXEDFILEINFO *fixedVerInfo = NULL;
			
			if(VerQueryValueW(verInfo, L"\\", (void**)&fixedVerInfo, &fixedVerInfoSize))
			{
				if(fixedVerInfo->dwFileType == VFT_DLL)
				{
					m_wmvCoreVersion[0] = HIWORD(fixedVerInfo->dwFileVersionMS);
					m_wmvCoreVersion[1] = LOWORD(fixedVerInfo->dwFileVersionMS);
					m_wmvCoreVersion[2] = HIWORD(fixedVerInfo->dwFileVersionLS);
					m_wmvCoreVersion[3] = LOWORD(fixedVerInfo->dwFileVersionLS);
				}
			}
		}
		
		SAFE_DELETE_ARRAY(verInfo);
	}

	WMCreateSyncReaderProc pWMCreateSyncReader = reinterpret_cast<WMCreateSyncReaderProc>(GetProcAddress(m_wmvCore, "WMCreateSyncReader"));

	if(!(pWMCreateSyncReader != NULL))
	{
		throw "Fatal Error: Entry point 'WMVCORE.DLL::WMCreateSyncReader' not found!\nWindows Media Format Runtime (Version 9+) is required.";
	}

	if(pWMCreateSyncReader(NULL, 0, &m_reader) != S_OK)
	{
		m_reader = NULL;
		throw "Fatal Error: Failed to create IWMSyncReader interface!\nWindows Media Format Runtime (Version 9+) is required.";
	}
}

CWmaReader::~CWmaReader(void)
{
	if(m_reader)
	{
		m_reader->Release();
		m_reader = NULL;
	}
	if(VALID_HANDLE(m_wmvCore))
	{
		FreeLibrary(m_wmvCore);
		m_wmvCore = NULL;
	}
}

bool CWmaReader::getRuntimeVersion(wchar_t *version, size_t size)
{
	if(m_wmvCoreVersion[0] || m_wmvCoreVersion[1] || m_wmvCoreVersion[2] || m_wmvCoreVersion[3])
	{
		_snwprintf_s(version, size, _TRUNCATE, L"%u.%u.%u.%u", m_wmvCoreVersion[0], m_wmvCoreVersion[1], m_wmvCoreVersion[2], m_wmvCoreVersion[3]);
		if(m_wmvCoreVersion[0] < 9) wcsncat_s(version, size, L" (UNSUPPORTED)", _TRUNCATE);
		return true;
	}
	else
	{
		_snwprintf_s(version, size, _TRUNCATE, L"N/A");
		return false;
	}
}

bool CWmaReader::isValid(const wchar_t *filename)
{
	WMValidateDataProc pWMValidateData = reinterpret_cast<WMValidateDataProc>(GetProcAddress(m_wmvCore, "WMValidateData"));	

	if(!(pWMValidateData != NULL))
	{
		return true;
	}

	bool isValid = false;

	BYTE *data;
	FILE *file = NULL;
	DWORD size = 0;

	if(pWMValidateData(NULL, &size) != S_OK)
	{
		return false;
	}

	data = new BYTE[size];

	if(_wfopen_s(&file, filename, L"rb"))
	{
		SAFE_DELETE_ARRAY(data);
		return false;
	}

	size_t len = fread(data, 1, static_cast<size_t>(size), file);

	if(len != static_cast<size_t>(size))
	{
		fclose(file);
		file = NULL;
		SAFE_DELETE_ARRAY(data);
		return false;
	}

	HRESULT result = pWMValidateData(data, &size);

	switch(result)
	{
	case S_OK:
		isValid = true;
		break;
	case NS_E_INVALID_DATA:
		isValid = false;
		break;
	case ASF_E_BUFFERTOOSMALL:
		isValid = true;
		break;
	default:
		isValid = false;
		break;
	}

	fclose(file);
	file = NULL;
	SAFE_DELETE_ARRAY(data);
	return isValid;
}

bool CWmaReader::isProtected(const wchar_t *filename)
{
	WMIsContentProtectedProc pWMIsContentProtected = reinterpret_cast<WMIsContentProtectedProc>(GetProcAddress(m_wmvCore, "WMIsContentProtected"));	

	if(!(pWMIsContentProtected != NULL))
	{
		return false;
	}

	BOOL flag = FALSE;
	bool isProtected = true;
		
	HRESULT result = pWMIsContentProtected(filename, &flag);

	switch(result)
	{
	case S_FALSE:
		isProtected = false;
		break;
	case S_OK:
		isProtected = (flag == TRUE);
		break;
	default:
		isProtected = false;
		break;
	}

	return isProtected;
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
		m_isAnalyzed = false;
		return true;
	}

	return false;
}

void CWmaReader::close(void)
{
	if(m_isOpen)
	{
		m_reader->Close();
		m_isAnalyzed = false;
		m_isOpen = false;
	}
}

bool CWmaReader::analyze(WAVEFORMATEX *format)
{
	SecureZeroMemory(format, sizeof(WAVEFORMATEX));
	
	if((!m_isOpen) || m_isAnalyzed)
	{
		return false;
	}

	if(_findAudioStream(format))
	{
		BOOL isCompressed = TRUE;
		m_reader->SetReadStreamSamples(m_streamNum, FALSE);
		
		if(m_reader->GetReadStreamSamples(m_streamNum, &isCompressed) == S_OK)
		{
			if(!(isCompressed))
			{
				DWORD outputNum = 0;
		
				if(m_reader->GetOutputNumberForStream(m_streamNum, &outputNum) == S_OK)
				{
					m_isAnalyzed = true;
					m_outputNum = outputNum;
					return true;
				}
			}
		}
	}

	return false;
}

bool CWmaReader::_findAudioStream(WAVEFORMATEX *format)
{
	bool foundAudioStream = false;
	IWMProfile *pIWMProfile = NULL;

	if(m_reader->QueryInterface(IID_IWMProfile, (void**)&pIWMProfile) == S_OK)
	{
		DWORD streamCount = 0;

		for(WORD i = 1; i < 64; i++)
		{
			IWMStreamConfig *pIWMStreamConfig = NULL;
		
			if(pIWMProfile->GetStreamByNumber(i, &pIWMStreamConfig) == S_OK)
			{
				GUID streamType = WMMEDIATYPE_Text;
				
				if(pIWMStreamConfig->GetStreamType(&streamType) == S_OK)
				{
					if(streamType == WMMEDIATYPE_Audio)
					{
						IWMMediaProps *pIWMMediaProps = NULL;

						if(pIWMStreamConfig->QueryInterface(IID_IWMMediaProps, (void**)&pIWMMediaProps) == S_OK)
						{
							DWORD mediaTypeSize = 0;

							if(pIWMMediaProps->GetMediaType(NULL, &mediaTypeSize) == S_OK)
							{
								char *buffer = new char[mediaTypeSize];
								WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);
				
								if(pIWMMediaProps->GetMediaType(mediaType, &mediaTypeSize) == S_OK)
								{
									if(mediaType->formattype == WMFORMAT_WaveFormatEx)
									{
										m_streamNum = i;
										memcpy(format, mediaType->pbFormat, sizeof(WAVEFORMATEX));
										foundAudioStream = true;
									}
								}

								delete [] buffer;
							}
				
							pIWMMediaProps->Release();
							pIWMMediaProps = NULL;
						}
					}
				}

				pIWMStreamConfig->Release();
				pIWMStreamConfig = NULL;
			}

			if(foundAudioStream) break;
		}

		pIWMProfile->Release();
		pIWMProfile = NULL;
	}

	return foundAudioStream;
}

bool CWmaReader::configureOutput(WAVEFORMATEX *format)
{
	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}
	
	bool success = false;
	IWMOutputMediaProps *pIWMOutputMediaProps = NULL;
			
	if(format->nChannels > 2)
	{
		BOOL discreteOutput = TRUE;
		DWORD speakerCfg = 0x00000000; //DSSPEAKER_DIRECTOUT
		m_reader->SetOutputSetting(m_outputNum, g_wszEnableDiscreteOutput, WMT_TYPE_BOOL, (BYTE*)&discreteOutput, sizeof(BOOL));
		m_reader->SetOutputSetting(m_outputNum, g_wszSpeakerConfig, WMT_TYPE_DWORD, (BYTE*)&speakerCfg, sizeof(BOOL));
	}

	if(m_reader->GetOutputProps(m_outputNum, &pIWMOutputMediaProps) == S_OK)
	{
		DWORD mediaTypeSize = 0;

		if(pIWMOutputMediaProps->GetMediaType(NULL, &mediaTypeSize) == S_OK)
		{
			char *buffer =  new char[mediaTypeSize];
			WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);
				
			if(pIWMOutputMediaProps->GetMediaType(mediaType, &mediaTypeSize) == S_OK)
			{
				if(mediaType->formattype == WMFORMAT_WaveFormatEx)
				{
					memcpy(mediaType->pbFormat, format, sizeof(WAVEFORMATEX));

					if(pIWMOutputMediaProps->SetMediaType(mediaType) == S_OK)
					{
						if(m_reader->SetOutputProps(m_outputNum, pIWMOutputMediaProps) == S_OK)
						{
							success = true;
						}
					}
				}
			}
				
			SAFE_DELETE_ARRAY(buffer)
		}
				
		pIWMOutputMediaProps->Release();
		pIWMOutputMediaProps = NULL;
	}

	return success;
}

bool CWmaReader::getOutputFormat(WAVEFORMATEX *format)
{
	SecureZeroMemory(format, sizeof(WAVEFORMATEX));

	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}

	bool success = false;
	IWMOutputMediaProps *pIWMOutputMediaProps = NULL;
			
	if(m_reader->GetOutputProps(m_outputNum, &pIWMOutputMediaProps) == S_OK)
	{
		DWORD mediaTypeSize = 0;

		if(pIWMOutputMediaProps->GetMediaType(NULL, &mediaTypeSize) == S_OK)
		{
			char *buffer =  new char[mediaTypeSize];
			WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);
				
			if(pIWMOutputMediaProps->GetMediaType(mediaType, &mediaTypeSize) == S_OK)
			{
				if(mediaType->formattype == WMFORMAT_WaveFormatEx)
				{
					memcpy(format, mediaType->pbFormat, sizeof(WAVEFORMATEX));
					success = true;
				}
			}
				
			SAFE_DELETE_ARRAY(buffer)
		}
				
		pIWMOutputMediaProps->Release();
		pIWMOutputMediaProps = NULL;
	}

	return success;
}

double CWmaReader::getDuration(void)
{
	double duration = -1.0;

	if(!(m_isOpen && m_isAnalyzed))
	{
		return duration;
	}
	
	IWMHeaderInfo* pHdrInfo = NULL;
	
	if(m_reader->QueryInterface(IID_IWMHeaderInfo,(void**)&pHdrInfo) == S_OK)
	{
		WMT_ATTR_DATATYPE dType;
		WORD size = 0;
		WORD stream = 0; //m_streamNum

		if(pHdrInfo->GetAttributeByName(&stream, g_wszWMDuration, &dType, NULL, &size) == S_OK)
		{
			if((dType == WMT_TYPE_QWORD) && (size == sizeof(QWORD)))
			{
				BYTE pValue[sizeof(QWORD)];

				if(pHdrInfo->GetAttributeByName(&stream, g_wszWMDuration, &dType, (BYTE*)&pValue, &size) == S_OK)
				{
					duration = NANOTIME_TO_DOUBLE(*reinterpret_cast<QWORD*>(pValue));
				}
			}
		}
		
		pHdrInfo->Release();
		pHdrInfo = NULL;
	}

	return duration;
}

bool CWmaReader::getCodecInfo(wchar_t *codecName, wchar_t *codecInfo, size_t size)
{
	wcsncpy_s(codecName, size, L"Unknown", _TRUNCATE);
	wcsncpy_s(codecInfo, size, L"Unknown", _TRUNCATE);
	
	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}

	wchar_t *temp = NULL;
	IWMHeaderInfo2* pHdrInfo = NULL;
	bool foundInfo = false;

	if(m_reader->QueryInterface(IID_IWMHeaderInfo2,(void**)&pHdrInfo) == S_OK)
	{
		DWORD codecCount = 0;

		if(pHdrInfo->GetCodecInfoCount(&codecCount) == S_OK)
		{
			for(DWORD i = 0; i < codecCount; i++)
			{
				WORD sizeName = 0;
				WORD sizeDesc = 0;
				WORD sizeInfo = 0;
				WMT_CODEC_INFO_TYPE codecInfoType;

				if(pHdrInfo->GetCodecInfo(i, &sizeName, NULL, &sizeDesc, NULL, &codecInfoType, &sizeInfo, NULL) == S_OK)
				{
					if(codecInfoType == WMT_CODECINFO_AUDIO)
					{
						wchar_t *buffName = new wchar_t[sizeName+1];
						wchar_t *buffDesc = new wchar_t[sizeDesc+1];
						BYTE *buffInfo = new BYTE[sizeInfo];

						if(pHdrInfo->GetCodecInfo(i, &sizeName, buffName, &sizeDesc, buffDesc, &codecInfoType, &sizeInfo, buffInfo) == S_OK)
						{
							if(wcslen(buffName) > 0) wcsncpy_s(codecName, size, buffName, _TRUNCATE);
							if(wcslen(buffDesc) > 0) wcsncpy_s(codecInfo, size, buffDesc, _TRUNCATE);
							foundInfo = true;
						}

						SAFE_DELETE_ARRAY(buffName);
						SAFE_DELETE_ARRAY(buffDesc);
						SAFE_DELETE_ARRAY(buffInfo);
					}
				}
			}
		}
		
		pHdrInfo->Release();
		pHdrInfo = NULL;
	}

	return foundInfo;
}

bool CWmaReader::getTitle(wchar_t *title, size_t size)
{
	wcsncpy_s(title, size, L"Unknown", _TRUNCATE);
	
	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}
	
	IWMHeaderInfo* pHdrInfo = NULL;
	bool foundInfo = false;
	
	if(m_reader->QueryInterface(IID_IWMHeaderInfo,(void**)&pHdrInfo) == S_OK)
	{
		WMT_ATTR_DATATYPE dType;
		WORD attrSize = 0;
		WORD stream = 0; //m_streamNum;
		
		if(pHdrInfo->GetAttributeByName(&stream, g_wszWMTitle, &dType, NULL, &attrSize) == S_OK)
		{
			if((dType == WMT_TYPE_STRING))
			{
				size_t strLen = (attrSize / sizeof(wchar_t));
				
				if(strLen > 1)
				{
					wchar_t *temp = new wchar_t[strLen+1];
				
					if(pHdrInfo->GetAttributeByName(&stream, g_wszWMTitle, &dType, reinterpret_cast<BYTE*>(temp), &attrSize) == S_OK)
					{
						wcsncpy_s(title, size, temp, _TRUNCATE);
						foundInfo = true;
					}

					SAFE_DELETE_ARRAY(temp);
				}
			}
		}
		
		pHdrInfo->Release();
		pHdrInfo = NULL;
	}

	return foundInfo;
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

bool CWmaReader::getNextSample(BYTE *output, const size_t size, size_t *length, double *timeStamp, double *sampleDuration)
{
	*length = 0;
	if(timeStamp) *timeStamp = -1.0;
	if(sampleDuration) *sampleDuration = -1.0;
	SecureZeroMemory(output, size);

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

	if(bufferLen > size)
	{
		buffer->Release();
		return false;
	}

	memcpy(output, bufferPtr, bufferLen);
	*length = bufferLen;
	
	if(timeStamp) *timeStamp = NANOTIME_TO_DOUBLE(time);
	if(sampleDuration) *sampleDuration = NANOTIME_TO_DOUBLE(duration);
	
	buffer->Release();
	buffer = NULL;
	
	return true;
}
