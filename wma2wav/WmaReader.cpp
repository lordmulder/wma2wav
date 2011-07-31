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

///////////////////////////////////////////////////////////////////////////////

using namespace std;

typedef HRESULT (__stdcall *WMCreateSyncReaderProc)(IUnknown* pUnkCert, DWORD dwRights, IWMSyncReader **ppSyncReader);
typedef HRESULT (__stdcall *WMIsContentProtectedProc)(const WCHAR *pwszFileName, BOOL *pfIsProtected);
typedef HRESULT (__stdcall *WMValidateDataProc)(BYTE *pbData, DWORD *pdwDataSize);

///////////////////////////////////////////////////////////////////////////////

#define IF_OK(EXPR) if((EXPR) == S_OK)
#define IF_FAIL(EXPR) if((EXPR) != S_OK)

#define NANOTIME_TO_DOUBLE(T) (static_cast<double>((T) / 1000) / 10000.0)

///////////////////////////////////////////////////////////////////////////////

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

	dbg_printf(L"CWmaReader::CWmaReader: Initializing WMVCORE.DLL");

	if(!(secure_load_library(&m_wmvCore, L"wmvcore.dll")))
	{
		dbg_printf(L"CWmaReader::CWmaReader: LoadLibrary for WMVCORE.DLL has failed!");
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

	if(m_wmvCoreVersion[0] || m_wmvCoreVersion[1] || m_wmvCoreVersion[2] || m_wmvCoreVersion[3])
	{
		dbg_printf(L"CWmaReader::CWmaReader: WMVCORE.DLL v%u.%u.%u.%u loaded successfully", m_wmvCoreVersion[0], m_wmvCoreVersion[1], m_wmvCoreVersion[2], m_wmvCoreVersion[3]);
	}
	else
	{
		dbg_printf(L"CWmaReader::CWmaReader: WMVCORE.DLL loaded successfully, but version cannot be detected!");
	}

	WMCreateSyncReaderProc pWMCreateSyncReader = reinterpret_cast<WMCreateSyncReaderProc>(GetProcAddress(m_wmvCore, "WMCreateSyncReader"));

	if(!(pWMCreateSyncReader != NULL))
	{
		dbg_printf(L"CWmaReader::CWmaReader: Entry point 'WMCreateSyncReader' could not be resolved in WMVCORE.DLL");
		throw "Fatal Error: Entry point 'WMVCORE.DLL::WMCreateSyncReader' not found!\nWindows Media Format Runtime (Version 9+) is required.";
	}

	IF_FAIL(pWMCreateSyncReader(NULL, 0, &m_reader))
	{
		m_reader = NULL;
		dbg_printf(L"CWmaReader::CWmaReader: Function 'WMCreateSyncReader' has failed, reader could not be created");
		throw "Fatal Error: Failed to create IWMSyncReader interface!\nWindows Media Format Runtime (Version 9+) is required.";
	}

	dbg_printf(L"CWmaReader::CWmaReader: IWMSyncReader was created successfully");
}

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

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

	IF_FAIL(pWMValidateData(NULL, &size))
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

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

bool CWmaReader::open(const wchar_t *filename)
{
	if(m_isOpen)
	{
		return false;
	}

	IF_OK(m_reader->Open(filename))
	{
		m_isOpen = true;
		m_isAnalyzed = false;
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

void CWmaReader::close(void)
{
	if(m_isOpen)
	{
		m_reader->Close();
		m_isAnalyzed = false;
		m_isOpen = false;
	}
}

///////////////////////////////////////////////////////////////////////////////

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
		
		IF_OK(m_reader->GetReadStreamSamples(m_streamNum, &isCompressed))
		{
			if(!(isCompressed))
			{
				DWORD outputNum = 0;
				dbg_printf(L"CWmaReader::analyze: Stream does NOT deliver compressed output samples");
		
				IF_OK(m_reader->GetOutputNumberForStream(m_streamNum, &outputNum))
				{
					dbg_printf(L"CWmaReader::analyze: Output number for stream #%d is %u, analysis completed", static_cast<int>(m_streamNum), outputNum);
					m_isAnalyzed = true;
					m_outputNum = outputNum;
					return true;
				}
				else
				{
					dbg_printf(L"CWmaReader::analyze: Failed to obtain output number for stream #%d!", static_cast<int>(m_streamNum), outputNum);
				}
			}
			else
			{
				dbg_printf(L"CWmaReader::analyze: Stream is configured to deliver compressed samples, rejected!");
			}
		}
		else
		{
			dbg_printf(L"CWmaReader::analyze: Failed to determine whether stream delivers compressed samples!");
		}
	}
	else
	{
		dbg_printf(L"CWmaReader::analyze: Audio stream not found, I give up!");
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool CWmaReader::_findAudioStream(WAVEFORMATEX *format)
{
	bool foundAudioStream = false;
	IWMProfile *pIWMProfile = NULL;

	IF_FAIL(m_reader->QueryInterface(IID_IWMProfile, (void**)&pIWMProfile))
	{
		dbg_printf(L"CWmaReader::analyze: Failed to query IWMProfile interface!");
		return false;
	}
	
	for(WORD i = 1; i < 64; i++)
	{
		IWMStreamConfig *pIWMStreamConfig = NULL;
		dbg_printf(L"CWmaReader::analyze: Analyzing stream #%d", static_cast<int>(i));
		
		IF_OK(pIWMProfile->GetStreamByNumber(i, &pIWMStreamConfig))
		{
			GUID streamType = WMMEDIATYPE_Text;
			dbg_printf(L"CWmaReader::analyze: Stream configuration of stream #%d read successfully", static_cast<int>(i));
				
			IF_OK(pIWMStreamConfig->GetStreamType(&streamType))
			{
				dbg_printf(L"CWmaReader::analyze: Stream type of stream #%d read successfully", static_cast<int>(i));

				if(streamType == WMMEDIATYPE_Audio)
				{
					dbg_printf(L"CWmaReader::analyze: Stream #%d appears to be an audio stream", static_cast<int>(i));
					foundAudioStream = _testAudioStream(format, i, pIWMStreamConfig);
				}
				else
				{
					dbg_printf(L"CWmaReader::analyze: Not an audio stream, skipping this stream!");
				}
			}
			else
			{
				dbg_printf(L"CWmaReader::analyze: Failed to read stream type of stream #%d!", static_cast<int>(i));
			}

			pIWMStreamConfig->Release();
			pIWMStreamConfig = NULL;
		}
		else
		{
			dbg_printf(L"CWmaReader::analyze: Failed to read stream configuration of stream #%d!", static_cast<int>(i));
		}

		if(foundAudioStream) break;
	}

	pIWMProfile->Release();
	pIWMProfile = NULL;

	return foundAudioStream;
}

///////////////////////////////////////////////////////////////////////////////

bool CWmaReader::_testAudioStream(WAVEFORMATEX *format, const WORD idx, IWMStreamConfig *pIWMStreamConfig)
{
	bool audioStreamOkay = false;
	IWMMediaProps *pIWMMediaProps = NULL;

	IF_FAIL(pIWMStreamConfig->QueryInterface(IID_IWMMediaProps, (void**)&pIWMMediaProps))
	{
		dbg_printf(L"CWmaReader::analyze: Failed to query IWMMediaProps interface for stream #%d!", static_cast<int>(idx));
		return false;
	}

	DWORD mediaTypeSize = 0;
	dbg_printf(L"CWmaReader::analyze: Got IWMMediaProps interface for stream #%d", static_cast<int>(idx));

	IF_OK(pIWMMediaProps->GetMediaType(NULL, &mediaTypeSize))
	{
		char *buffer = new char[mediaTypeSize];
		WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);
				
		IF_OK(pIWMMediaProps->GetMediaType(mediaType, &mediaTypeSize))
		{
			dbg_printf(L"CWmaReader::analyze: Media type of stream #%d read successfully", static_cast<int>(idx));
									
			if(mediaType->formattype == WMFORMAT_WaveFormatEx)
			{
				dbg_printf(L"CWmaReader::analyze: Format type of stream #%d is WAVEFORMATEX", static_cast<int>(idx));
				memcpy(format, mediaType->pbFormat, sizeof(WAVEFORMATEX));
				m_streamNum = idx;
				audioStreamOkay = true;
			}
			else
			{
				dbg_printf(L"CWmaReader::analyze: Format type of stream #%d is unsupported!", static_cast<int>(idx));
			}
		}
		else
		{
			dbg_printf(L"CWmaReader::analyze: Failed to read media type of stream #%d!", static_cast<int>(idx));
		}

		delete [] buffer;
	}
	else
	{
		dbg_printf(L"CWmaReader::analyze: Failed to read size of media type of stream #%d!", static_cast<int>(idx));
	}

	pIWMMediaProps->Release();
	pIWMMediaProps = NULL;

	return audioStreamOkay;
}


///////////////////////////////////////////////////////////////////////////////

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
		dbg_printf(L"CWmaReader::configureOutput: Trying to enable 'true' multi-channel output");
		BOOL discreteOutput = TRUE;
		DWORD speakerCfg = 0x00000000; //DSSPEAKER_DIRECTOUT
		m_reader->SetOutputSetting(m_outputNum, g_wszEnableDiscreteOutput, WMT_TYPE_BOOL, (BYTE*)&discreteOutput, sizeof(BOOL));
		m_reader->SetOutputSetting(m_outputNum, g_wszSpeakerConfig, WMT_TYPE_DWORD, (BYTE*)&speakerCfg, sizeof(BOOL));
	}

	IF_OK(m_reader->GetOutputProps(m_outputNum, &pIWMOutputMediaProps))
	{
		DWORD mediaTypeSize = 0;
		dbg_printf(L"CWmaReader::configureOutput: Got current output media properties");

		IF_OK(pIWMOutputMediaProps->GetMediaType(NULL, &mediaTypeSize))
		{
			char *buffer =  new char[mediaTypeSize];
			WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);

			IF_OK(pIWMOutputMediaProps->GetMediaType(mediaType, &mediaTypeSize))
			{
				dbg_printf(L"CWmaReader::configureOutput: Got current output media type");
	
				if(mediaType->formattype == WMFORMAT_WaveFormatEx)
				{
					dbg_printf(L"CWmaReader::configureOutput: Media format type is WAVEFORMATEX");
					memcpy(mediaType->pbFormat, format, sizeof(WAVEFORMATEX));

					IF_OK(pIWMOutputMediaProps->SetMediaType(mediaType))
					{
						dbg_printf(L"CWmaReader::configureOutput: New output media type applied successfully");

						IF_OK(m_reader->SetOutputProps(m_outputNum, pIWMOutputMediaProps))
						{
							dbg_printf(L"CWmaReader::configureOutput: New output media properties applied successfully");
							success = true;
						}
						else
						{
							dbg_printf(L"CWmaReader::configureOutput: Failed to apply new output media properties!");
						}
					}
					else
					{
						dbg_printf(L"CWmaReader::configureOutput: Failed to apply new output media type!");
					}
				}
				else
				{
					dbg_printf(L"CWmaReader::configureOutput: Found an unsupported media format type!");
				}
			}
				
			SAFE_DELETE_ARRAY(buffer)
		}
				
		pIWMOutputMediaProps->Release();
		pIWMOutputMediaProps = NULL;
	}
	else
	{
		dbg_printf(L"CWmaReader::configureOutput: Failed to get current output media properties!");
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////

bool CWmaReader::getOutputFormat(WAVEFORMATEX *format)
{
	SecureZeroMemory(format, sizeof(WAVEFORMATEX));

	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}

	bool success = false;
	IWMOutputMediaProps *pIWMOutputMediaProps = NULL;
			
	IF_OK(m_reader->GetOutputProps(m_outputNum, &pIWMOutputMediaProps))
	{
		DWORD mediaTypeSize = 0;
		dbg_printf(L"CWmaReader::getOutputFormat: Output media properties read successfully");

		IF_OK(pIWMOutputMediaProps->GetMediaType(NULL, &mediaTypeSize))
		{
			char *buffer =  new char[mediaTypeSize];
			WM_MEDIA_TYPE *mediaType = reinterpret_cast<WM_MEDIA_TYPE*>(buffer);
				
			IF_OK(pIWMOutputMediaProps->GetMediaType(mediaType, &mediaTypeSize))
			{
				dbg_printf(L"CWmaReader::getOutputFormat: Output media type read successfully");

				if(mediaType->formattype == WMFORMAT_WaveFormatEx)
				{
					dbg_printf(L"CWmaReader::getOutputFormat: Output media format read successfully");
					memcpy(format, mediaType->pbFormat, sizeof(WAVEFORMATEX));
					success = true;
				}
				else
				{
					dbg_printf(L"CWmaReader::getOutputFormat: Output media format type is unsupported!");
				}
			}
			else
			{
				dbg_printf(L"CWmaReader::getOutputFormat: Failed to get output media type!");
			}
				
			SAFE_DELETE_ARRAY(buffer)
		}
		else
		{
			dbg_printf(L"CWmaReader::getOutputFormat: Failed to get output media type!");
		}
				
		pIWMOutputMediaProps->Release();
		pIWMOutputMediaProps = NULL;
	}
	else
	{
		dbg_printf(L"CWmaReader::getOutputFormat: Failed to read output media properties!");
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////

double CWmaReader::getDuration(void)
{
	double duration = -1.0;

	if(!(m_isOpen && m_isAnalyzed))
	{
		return duration;
	}
	
	IWMHeaderInfo* pHdrInfo = NULL;
	
	IF_OK(m_reader->QueryInterface(IID_IWMHeaderInfo,(void**)&pHdrInfo))
	{
		WMT_ATTR_DATATYPE dType;
		WORD size = 0;
		WORD stream = 0; //m_streamNum
		
		dbg_printf(L"CWmaReader::getDuration: Got IWMHeaderInfo interface");

		IF_OK(pHdrInfo->GetAttributeByName(&stream, g_wszWMDuration, &dType, NULL, &size))
		{
			if((dType == WMT_TYPE_QWORD) && (size == sizeof(QWORD)))
			{
				BYTE pValue[sizeof(QWORD)];
				dbg_printf(L"CWmaReader::getDuration: Found attribute '%s' of expected type/size", g_wszWMDuration);

				IF_OK(pHdrInfo->GetAttributeByName(&stream, g_wszWMDuration, &dType, (BYTE*)&pValue, &size))
				{
					dbg_printf(L"CWmaReader::getDuration: Attribute '%s' read successfully", g_wszWMDuration);
					duration = NANOTIME_TO_DOUBLE(*reinterpret_cast<QWORD*>(pValue));
				}
				else
				{
					dbg_printf(L"CWmaReader::getDuration: Failed to read '%s' attribute!", g_wszWMDuration);
				}
			}
			else
			{
				dbg_printf(L"CWmaReader::getDuration: Attribute does not match the expected type/size!");
			}
		}
		else
		{
			dbg_printf(L"CWmaReader::getDuration: Attribute '%s' could not be found!", g_wszWMDuration);
		}
		
		pHdrInfo->Release();
		pHdrInfo = NULL;
	}
	else
	{
		dbg_printf(L"CWmaReader::getDuration: Failed to query IWMHeaderInfo interface!");
	}

	return duration;
}

///////////////////////////////////////////////////////////////////////////////

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

	IF_OK(m_reader->QueryInterface(IID_IWMHeaderInfo2,(void**)&pHdrInfo))
	{
		DWORD codecCount = 0;

		IF_OK(pHdrInfo->GetCodecInfoCount(&codecCount))
		{
			for(DWORD i = 0; i < codecCount; i++)
			{
				WORD sizeName = 0;
				WORD sizeDesc = 0;
				WORD sizeInfo = 0;
				WMT_CODEC_INFO_TYPE codecInfoType;

				IF_OK(pHdrInfo->GetCodecInfo(i, &sizeName, NULL, &sizeDesc, NULL, &codecInfoType, &sizeInfo, NULL))
				{
					if(codecInfoType == WMT_CODECINFO_AUDIO)
					{
						wchar_t *buffName = new wchar_t[sizeName+1];
						wchar_t *buffDesc = new wchar_t[sizeDesc+1];
						BYTE *buffInfo = new BYTE[sizeInfo];

						IF_OK(pHdrInfo->GetCodecInfo(i, &sizeName, buffName, &sizeDesc, buffDesc, &codecInfoType, &sizeInfo, buffInfo))
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

///////////////////////////////////////////////////////////////////////////////

bool CWmaReader::getTitle(wchar_t *title, size_t size)
{
	wcsncpy_s(title, size, L"Unknown", _TRUNCATE);
	
	if(!(m_isOpen && m_isAnalyzed))
	{
		return false;
	}
	
	IWMHeaderInfo* pHdrInfo = NULL;
	bool foundInfo = false;
	
	IF_OK(m_reader->QueryInterface(IID_IWMHeaderInfo,(void**)&pHdrInfo))
	{
		WMT_ATTR_DATATYPE dType;
		WORD attrSize = 0;
		WORD stream = 0; //m_streamNum;
		
		IF_OK(pHdrInfo->GetAttributeByName(&stream, g_wszWMTitle, &dType, NULL, &attrSize))
		{
			if((dType == WMT_TYPE_STRING))
			{
				size_t strLen = (attrSize / sizeof(wchar_t));
				
				if(strLen > 1)
				{
					wchar_t *temp = new wchar_t[strLen+1];
				
					IF_OK(pHdrInfo->GetAttributeByName(&stream, g_wszWMTitle, &dType, reinterpret_cast<BYTE*>(temp), &attrSize))
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

///////////////////////////////////////////////////////////////////////////////

size_t CWmaReader::getSampleSize(void)
{
	DWORD streamMax = 0;
	DWORD outputMax = 0;
	
	IF_OK(m_reader->GetMaxOutputSampleSize(m_outputNum, &outputMax))
	{
		dbg_printf(L"CWmaReader::getSampleSize: Maximum sample size for output #%u read successfully", m_outputNum);

		IF_OK(m_reader->GetMaxStreamSampleSize(m_streamNum, &streamMax))
		{
			dbg_printf(L"CWmaReader::getSampleSize: Maximum sample size for stream #%d read successfully", static_cast<int>(m_streamNum));
			dbg_printf(L"CWmaReader::getSampleSize: Maximum stream/output sample size is: %u", max(outputMax, streamMax));
			return max(outputMax, streamMax);
		}
		else
		{
			dbg_printf(L"CWmaReader::getSampleSize: Failed to read maximum sample size for stream #%d!", static_cast<int>(m_streamNum));
		}
	}
	else
	{
		dbg_printf(L"CWmaReader::getSampleSize: Failed to read maximum sample size for output #%u!", m_outputNum);
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

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

	IF_FAIL(result = m_reader->GetNextSample(m_streamNum, &buffer, &time, &duration, &flags, &sampleOutputNo, &sampleStreamNo))
	{
		dbg_printf(L"CWmaReader::getNextSample: Failed to read next sample (code 0x%X)", result);
		return (result == NS_E_NO_MORE_SAMPLES) ? true : false;
	}

	IF_FAIL(buffer->GetLength(&bufferLen))
	{
		dbg_printf(L"CWmaReader::getNextSample: Failed to get the length of the sample!");
		buffer->Release();
		return false;
	}

	IF_FAIL(buffer->GetBuffer(&bufferPtr))
	{
		dbg_printf(L"CWmaReader::getNextSample: Failed to get the pointer to the sample buffer!");
		buffer->Release();
		return false;
	}

	if(bufferLen > size)
	{
		dbg_printf(L"CWmaReader::getNextSample: Sample length exceeds destination buffer size!");
		buffer->Release();
		return false;
	}

	memcpy(output, bufferPtr, bufferLen);
	*length = bufferLen;
	
	if(timeStamp) *timeStamp = NANOTIME_TO_DOUBLE(time);
	if(sampleDuration) *sampleDuration = NANOTIME_TO_DOUBLE(duration);
	
	dbg_printf(L"CWmaReader::getNextSample: Sample read successfully (time: %I64u, duration: %I64u)", time, duration);

	buffer->Release();
	buffer = NULL;
	
	return true;
}
