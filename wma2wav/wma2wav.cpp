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

#include "stdafx.h"

#include "WmaReader.h"
#include "Utils.h"

#include <Objbase.h>
#include <io.h>

using namespace std;

// ----------------------------------------------------------------------------------------------------------

static bool parse_cli(int argc, _TCHAR* argv[], wchar_t **inputFile, wchar_t **outputFile, bool *overwrite, bool *rawOutput)
{
	*inputFile = NULL;
	*outputFile = NULL;
	*overwrite = false;
	*rawOutput = false;
	char *temp = NULL;

	for(int i = 1; i < argc; i++)
	{
		if(!_wcsicmp(argv[i], L"-i"))
		{
			if(i < (argc - 1))
			{
				*inputFile = argv[++i];
				continue;
			}
			else
			{
				if(temp = utf16_to_utf8(argv[i]))
				{
					fprintf(stderr, "Argument missing for command-line option:\n%s\n\n", temp);
					SAFE_DELETE_ARRAY(temp);
				}
				return false;
			}
		}
		if(!_wcsicmp(argv[i], L"-o"))
		{
			if(i < (argc - 1))
			{
				*outputFile = argv[++i];
				continue;
			}
			else
			{
				if(temp = utf16_to_utf8(argv[i]))
				{
					fprintf(stderr, "Argument missing for command-line option:\n%s\n\n", temp);
					SAFE_DELETE_ARRAY(temp);
				}
				return false;
			}
		}
		if(!_wcsicmp(argv[i], L"-f"))
		{
			*overwrite = true;
			continue;
		}
		if(!_wcsicmp(argv[i], L"-r"))
		{
			*rawOutput = true;
			continue;
		}
		
		if(temp = utf16_to_utf8(argv[i]))
		{
			fprintf(stderr, "Unknown command-line option:\n%s\n\n", temp);
			SAFE_DELETE_ARRAY(temp);
		}

		return false;
	}

	if(!((*inputFile) && (*outputFile)))
	{
		cerr << "Input and/or output file not specified!\n" << endl;
		return false;
	}

	if((!_wcsicmp(*outputFile, L"-")) && (!(*rawOutput)))
	{
		cerr << "Output to STDOUT requires \"raw\" mode -> switching to \"raw\" mode!\n" << endl;
		*rawOutput = true;
	}

	return true;
}

// ----------------------------------------------------------------------------------------------------------

static int wma2wav(int argc, _TCHAR* argv[])
{
	repair_standard_streams();
	
	cerr << "wma2wav - Dump WMA/WMV files to Wave Audio [" __DATE__ "]" << endl;
	cerr << "Copyright (c) 2011 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved." << endl;
	cerr << "Released under the terms of the GNU General Public License.\n" << endl;

	CWmaReader *wmaReader = NULL;
	WAVEFORMATEX format;
	SecureZeroMemory(&format, sizeof(WAVEFORMATEX));
	double duration = 0.0;
	BYTE *buffer = NULL;
	size_t bufferLen = 0;
	size_t sampleLen = 0;
	short indicator = 0;
	unsigned __int64 samplesTotal = 0;
	unsigned __int64 samplesCurrent = 0;
	unsigned __int64 bytesWritten = 0;
	unsigned int bytesPerSample = 0;
	wchar_t *inputFile = NULL;
	wchar_t *outputFile = NULL;
	FILE *writer = NULL;
	bool overwriteFlag = false;
	bool rawOutput = false;
	char *temp = NULL;

	if(!parse_cli(argc, argv, &inputFile, &outputFile, &overwriteFlag, &rawOutput))
	{
		cerr << "Usage:" << endl;
		cerr << "  wma2wav.exe [options] -i <input> -o <output>\n" << endl;
		cerr << "Options:" << endl;
		cerr << "  -i <input>   Select input ASF (WMA/WMV) file to read from" << endl;
		cerr << "  -o <output>  Select output Wave file to write to, specify \"-\" for STDOUT" << endl;
		cerr << "  -f           Force overwrite of output file (if already exists)" << endl;
		cerr << "  -r           Output \"raw\" PCM data to file instead of Wave/RIFF file\n" << endl;
		return 1;
	}

	if(!rawOutput)
	{
		cerr << "Wave/RIFF output not implemented yet, please use \"-r\" option for now ;-)\n" << endl;
		return 1;
	}

	if(CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK)
	{
		cerr << "Fatal Error: COM initialization has failed unexpectedly!" << endl;
		_exit(-1);
	}
	
	if(temp = utf16_to_utf8(inputFile))
	{
		fprintf(stderr, "Input file: %s\n", temp);
		SAFE_DELETE_ARRAY(temp);
	}
	if(temp = utf16_to_utf8(outputFile))
	{
		fprintf(stderr, "Output file: %s\n\n", temp);
		SAFE_DELETE_ARRAY(temp);
	}
	
	if(_waccess(inputFile, 4))
	{
		cerr << "Input file could not be found or access denied!" << endl;
		return 2;
	}
	if(_wcsicmp(outputFile, L"-") && (!overwriteFlag))
	{
		if(!_waccess(outputFile, 4))
		{
			cerr << "Output file already exists, will NOT overwrite!" << endl;
			return 3;
		}
	}

	wmaReader = new CWmaReader();
	cerr << "Opening input file... " << flush;

	if(!wmaReader->open(inputFile))
	{
		cerr << "Failed" << endl;
		delete wmaReader;
		return 4;
	}
	
	cerr << "OK\nAnalyzing input file... " << flush;

	if(!wmaReader->analyze())
	{
		cerr << "Failed" << endl;
		delete wmaReader;
		return 5;
	}
		
	if(!wmaReader->getFormat(&format))
	{
		cerr << "Internal error, aborting! (0x01)" << endl;
		_exit(-1);
	}

	cerr << "OK\n\n[Audio Properties]" << endl;
	cerr << "wFormatTag: " << hex << format.wFormatTag << dec << endl;
	cerr << "nChannels: " << format.nChannels << endl;
	cerr << "nSamplesPerSec: " << format.nSamplesPerSec << endl;
	cerr << "wBitsPerSample: " << format.wBitsPerSample << endl;

	if((duration = wmaReader->getDuration()) > 0.0)
	{
		cerr << "fDuration: " << duration << endl;
		samplesTotal = static_cast<unsigned int>(static_cast<double>(format.nSamplesPerSec) * duration);
	}
	
	if((bufferLen = wmaReader->getSampleSize()) < 1)
	{
		cerr << "\nFailed to detect maximum sample size!" << endl;
		SAFE_DELETE(wmaReader);
		return 6;
	}
	
	cerr << "nMaxSampleSize: " << bufferLen << endl;
	cerr << "\nOpening output file... " << flush;

	if(_wcsicmp(outputFile, L"-"))
	{
		if(_wfopen_s(&writer, outputFile, L"wb"))
		{
			cerr << "Failed" << endl;
			SAFE_DELETE(wmaReader);
			return 7;
		}
	}
	else
	{
		writer = stdout;
	}

	cerr << "OK\n" << endl;

	bufferLen = ((bufferLen / 4096) + 1) * 4096;
	buffer = new BYTE[bufferLen];
	bytesPerSample = (format.wBitsPerSample * format.nChannels) / 8;

	while(true)
	{
		if(!indicator)
		{
			if(samplesTotal > 0)
			{
				double completed = (static_cast<double>(samplesCurrent) / static_cast<double>(samplesTotal)) * 100.0;
				fprintf(stderr, "\r[%5.1f%%] %I64u of %I64u samples completed...", min(100.0, completed), samplesCurrent, max(samplesTotal, samplesCurrent));
			}
			else
			{
				fprintf(stderr, "\r%I64u samples completed...", samplesCurrent); \
			}
		}
		
		indicator = (indicator + 1 ) % 10;

		if(!wmaReader->getNextSample(buffer, &sampleLen))
		{
			cerr << "\n\nFailed to read sample from input file!" << endl;
			fclose(writer);
			SAFE_DELETE(wmaReader);
			SAFE_DELETE_ARRAY(buffer);
			return 8;
		}
		
		if(!(sampleLen > 0))
		{
			fprintf(stderr, "\r[%5.1f%%] %I64u of %I64u samples completed...", 100.0, samplesCurrent, samplesCurrent);
			cerr << "\n\nAll done." << endl;
			break;
		}

		if(fwrite(buffer, 1,  sampleLen, writer) < sampleLen)
		{
			cerr << "\n\nFailed to write sample to output file!" << endl;
			fclose(writer);
			SAFE_DELETE(wmaReader);
			SAFE_DELETE_ARRAY(buffer);
			return 9;
		}

		bytesWritten += sampleLen;
		samplesCurrent = bytesWritten / bytesPerSample;
	}

	fclose(writer);
	SAFE_DELETE(wmaReader);
	SAFE_DELETE_ARRAY(buffer);

	return 0;
}

// ----------------------------------------------------------------------------------------------------------

static int wmain2(int argc, _TCHAR* argv[])
{
	try
	{
		return wma2wav(argc, argv);
	}
	catch(std::bad_alloc err)
	{
		fprintf(stderr, "\nMemory allocation has failed, application will exit!\n");
		TerminateProcess(GetCurrentProcess(), -1);
		return -1;
	}
	catch(char *err)
	{
		fprintf(stderr, "\n%s\n", err);
		TerminateProcess(GetCurrentProcess(), -1);
		return -1;
	}
	catch(...)
	{
		fprintf(stderr, "\nUnhandeled exception error, application will exit!\n");
		TerminateProcess(GetCurrentProcess(), -1);
		return -1;
	}
}

int wmain(int argc, _TCHAR* argv[])
{
	__try
	{
		int result = wmain2(argc, argv);
		CoUninitialize();
		return result;
	}
	__except(1)
	{
		fprintf(stderr, "\nUnhandeled system exception, application will exit!\n");
		TerminateProcess(GetCurrentProcess(), -1);
		return -1;
	}
}
