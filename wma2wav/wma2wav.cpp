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
#include <Objbase.h>
#include <io.h>

using namespace std;

// ----------------------------------------------------------------------------------------------------------

static int wma2wav(int argc, _TCHAR* argv[])
{
	cerr << "wma2wav - Dump WMA files to Wave Audio [" __DATE__ "]" << endl;
	cerr << "Copyright (c) 2011 by LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved." << endl;
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

	if(argc < 3)
	{
		cerr << "Usage:" << endl;
		cerr << "  wma2wav.exe <input.wma> <output.pcm>\n" << endl;
		return 1;
	}

	if(CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK)
	{
		cerr << "Fatal Error: COM initialization has failed unexpectedly!" << endl;
		_exit(-1);
	}

	inputFile = argv[1];
	outputFile = argv[2];

	wcerr << "Input file: " << inputFile << endl;
	wcerr << "Output file: " << outputFile << L"\n" << endl;
	
	if(_waccess(inputFile, 4))
	{
		cerr << "Input file could not be found or access denied!" << endl;
		return 2;
	}
	if(_wcsicmp(outputFile, L"-"))
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
		delete wmaReader;
		return 6;
	}
	
	cerr << "nMaxSampleSize: " << bufferLen << endl;
	cerr << "\nOpening input file... " << flush;

	if(_wcsicmp(outputFile, L"-"))
	{
		if(_wfopen_s(&writer, outputFile, L"wb"))
		{
			cerr << "Failed" << endl;
			delete wmaReader;
			return 7;
		}
	}
	else
	{
		int hCrtStdOut = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), 0);
		if(hCrtStdOut >= 0)
		{
			writer = _fdopen(hCrtStdOut, "w");
		}
		if(!(writer != NULL))
		{
			cerr << "Failed" << endl;
			delete wmaReader;
			return 7;
		}
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
			delete wmaReader;
			delete [] buffer;
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
			delete wmaReader;
			delete [] buffer;
			return 9;
		}

		bytesWritten += sampleLen;
		samplesCurrent = bytesWritten / bytesPerSample;
	}

	fclose(writer);
	delete wmaReader;
	delete [] buffer;

	return 0;
}

// ----------------------------------------------------------------------------------------------------------

static int wmain2(int argc, _TCHAR* argv[])
{
	try
	{
		return wma2wav(argc, argv);
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
		fprintf(stderr, "\nUnhandeled exception error, application will exit!\n");
		TerminateProcess(GetCurrentProcess(), -1);
		return -1;
	}
}
