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

#include "wma2wav.h"
#include "WmaReader.h"
#include "WaveWriter.h"
#include "RawWriter.h"
#include "Utils.h"

#include <io.h>
#include <float.h>
#include <limits>

using namespace std;

// ==========================================================================================================

typedef struct
{
	bool overwriteFlag;
	bool rawOutput;
	bool silentMode;
	bool aggressiveMode;
	bool noCompensation;
	bool alternativeMode;
	bool defaultFormat;
	bool forceWaveOut;
	bool showHelp;
	wchar_t title[128];
	wchar_t codecName[128];
	wchar_t codecInfo[128];
	wchar_t rtVersion[128];
	wchar_t *inputFile;
	wchar_t *outputFile;
	double maxTime;
}
param_t;

static const wchar_t *txt_missingArgument = L"Argument missing for command-line option:\n%s\n\n";
static const char *alive = "|/-\\";

volatile bool g_aborted = false;

// ==========================================================================================================

static bool parse_cli(int argc, _TCHAR* argv[], param_t *param)
{
	param->inputFile = NULL;
	param->outputFile = NULL;
	param->overwriteFlag = false;
	param->rawOutput = false;
	param->silentMode = false;
	param->aggressiveMode = false;
	param->noCompensation = false;
	param->alternativeMode = false;
	param->defaultFormat = false;
	param->forceWaveOut = false;
	param->showHelp = false;
	param->maxTime = numeric_limits<double>::infinity();
	char *temp = NULL;

	//-------------------------------------------------------------------------
	// Parse command-line parameters
	//-------------------------------------------------------------------------

	for(int i = 1; i < argc; i++)
	{
		if(STREQ(argv[i], L"-i"))
		{
			if(i < (argc - 1))
			{
				param->inputFile = argv[++i];
				continue;
			}
			else
			{
				fwprintf(stderr, txt_missingArgument, argv[i]);
				return false;
			}
		}
		if(STREQ(argv[i], L"-o"))
		{
			if(i < (argc - 1))
			{
				param->outputFile = argv[++i];
				continue;
			}
			else
			{
				fwprintf(stderr, txt_missingArgument, argv[i]);
				return false;
			}
		}
		if(STREQ(argv[i], L"-t"))
		{
			if(i < (argc - 1))
			{
				param->maxTime = abs(_wtof(argv[++i]));
				continue;
			}
			else
			{
				fwprintf(stderr, txt_missingArgument, argv[i]);
				return false;
			}
		}
		if(STREQ(argv[i], L"-f"))
		{
			param->overwriteFlag = true;
			continue;
		}
		if(STREQ(argv[i], L"-r"))
		{
			param->rawOutput = true;
			continue;
		}
		if(STREQ(argv[i], L"-s"))
		{
			param->silentMode = true;
			continue;
		}
		if(STREQ(argv[i], L"-a"))
		{
			param->aggressiveMode = true;
			continue;
		}
		if(STREQ(argv[i], L"-n"))
		{
			param->noCompensation = true;
			continue;
		}
		if(STREQ(argv[i], L"-x"))
		{
			param->alternativeMode = true;
			continue;
		}
		if(STREQ(argv[i], L"-d"))
		{
			param->defaultFormat = true;
			continue;
		}
		if(STREQ(argv[i], L"-w"))
		{
			param->forceWaveOut = true;
			continue;
		}
		
		if(STREQ(argv[i], L"-h") || STREQ(argv[i], L"-?") || STREQ(argv[i], L"/?") || STREQ(argv[i], L"-help") || STREQ(argv[i], L"--help"))
		{
			param->showHelp = true;
			return true;
		}

		fwprintf(stderr, L"Unknown command-line option:\n%s\n\n", argv[i]);
		return false;
	}

	//-------------------------------------------------------------------------
	// Validate parameters
	//-------------------------------------------------------------------------
	
	if(param->noCompensation)
	{
		if(param->aggressiveMode)
		{
			wcerr << "Error: Can not use \"-a\" and \"-n\" options at the same time!\n" << endl;
			return false;
		}
		if(param->alternativeMode)
		{
			wcerr << "Error: Can not use \"-x\" and \"-n\" options at the same time!\n" << endl;
			return false;
		}
	}
	
	if(!((param->inputFile) && (param->outputFile)))
	{
		wcerr << "Error: Input and/or output file not specified!\n" << endl;
		return false;
	}

	if(STREQ(param->outputFile, L"-") && (!(param->rawOutput)) && (!(param->forceWaveOut)))
	{
		wcerr << "Output to STDOUT requires \"raw\" mode -> switching to \"raw\" mode ('-r' option)." << endl;
		wcerr << "If you want to output a fake(!) Wave/RIFF file to STDOUT, use the '-w' option!\n" << endl;
		param->rawOutput = true;
	}

	return true;
}

// ==========================================================================================================

int wma2wav(int argc, _TCHAR* argv[])
{
	wcerr << "wma2wav - Dump WMA/WMV files to uncompressed Wave Audio" << endl;
	wcerr << "Copyright (c) 2011 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved." << endl;
	wcerr << "Built on "__DATE__" at " __TIME__ " with " __COMPILER__ " for " __ARCH__ "\n" << endl;
	wcerr << "This program is free software; you can redistribute it and/or modify" << endl;
	wcerr << "it under the terms of the GNU General Public License <http://www.gnu.org/>." << endl;
	wcerr << "Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n" << endl;

	dbg_printf(L"wma2wav - Dump WMA/WMV files to uncompressed Wave Audio");
	dbg_printf(L"Built on %S at %S with %S for %S", __DATE__, __TIME__, __COMPILER__, __ARCH__);
	dbg_printf(L"CLI: %s", GetCommandLineW());

#if defined(_DEBUG) || !defined(NDEBUG)
	set_console_color(stderr, BACKGROUND_INTENSITY | BACKGROUND_RED);
	wcerr << "!!! DEBUG VERSION - DEBUG VERSION - DEBUG VERSION - DEBUG VERSION !!!\n" << endl;
	restore_console_color(stderr);
#endif

	CAbstractSink *sink = NULL;
	CWmaReader *wmaReader = NULL;
	param_t param;
	SecureZeroMemory(&param, sizeof(param_t));
	WAVEFORMATEX format;
	SecureZeroMemory(&format, sizeof(WAVEFORMATEX));
	double duration = -1.0;
	BYTE *buffer = NULL;
	size_t bufferLen = 0;
	size_t sampleLen = 0;
	double currentTime = 0.0;
	short indicator = 0;
	short aliveIndex = 0;
	size_t stats[2] = {0, 0};
	char *temp = NULL;

	if(!parse_cli(argc, argv, &param))
	{
		wcerr << "You can type 'wma2wav.exe -h' for a list of available command-line options.\n" << endl;
		return 1;
	}

	if(param.showHelp)
	{
		wcerr << "Usage:" << endl;
		wcerr << "  wma2wav.exe [options] -i <input> -o <output>\n" << endl;
		wcerr << "Options:" << endl;
		wcerr << "  -i <input>   Select input ASF (WMA/WMV) file to read from" << endl;
		wcerr << "  -o <output>  Select output Wave file to write to, specify \"-\" for STDOUT" << endl;
		wcerr << "  -t <time>    Maximum number of seconds to dump (will stop at maximum)" << endl;
		wcerr << "  -f           Force overwrite of output file, if file already exists" << endl;
		wcerr << "  -r           Output \"raw\" PCM data to file instead of Wave/RIFF file" << endl;
		wcerr << "  -s           Silent mode, do not display progress indicator" << endl;
		wcerr << "  -x           Use the \"alternative\" timestamp calculation mode" << endl;
		wcerr << "  -a           Enable \"aggressive\" sync correction mode (not recommended)" << endl;
		wcerr << "  -n           No sync correction (can not use with '-a' or '-x')" << endl;
		wcerr << "  -d           Use \"default\" audio output format (e.g. Stereo, 16-Bit)" << endl;
		wcerr << "  -w           Always try to output a Wave/RIFF file (for use with STDOUT)" << endl;
		wcerr << "  -h           Prints a list of available command-line options\n" << endl;
		wcerr << "Example:" << endl;
		wcerr << "  wma2wav.exe -i \"c:\\my music\\input.wma\" -o \"c:\\my music\\output.wav\"\n" << endl;
		return 0;
	}

	const double maxGapSize = (param.noCompensation) ? numeric_limits<double>::infinity() : ((param.aggressiveMode) ? 0.00000001 : 0.00100001);

	if(!(safe_com_init()))
	{
		wcerr << "Fatal Error: COM initialization has failed unexpectedly!" << endl;
		_exit(-1);
	}

	//-------------------------------------------------------------------------
	// Check for input/output file availability
	//-------------------------------------------------------------------------

	if(param.inputFile)
	{
		fwprintf(stderr, L"Input file:\t%s\n", param.inputFile);
	}
	if(!STREQ(param.outputFile, L"-"))
	{
		if(param.outputFile)
		{
			fwprintf(stderr, L"Output file:\t%s\n", param.outputFile);
		}
	}
	else
	{
		fwprintf(stderr, L"Output file:\t<STDOUT>\n");
	}
	
	if(_waccess(param.inputFile, 4))
	{
		wcerr << "\nError: Input file could not be found or access denied!" << endl;
		return 2;
	}
	if(!(STREQ(param.outputFile, L"-") || STREQ(param.outputFile, L"NUL") || param.overwriteFlag))
	{
		if(!_waccess(param.outputFile, 4))
		{
			wcerr << "\nError: Output file already exists, will NOT overwrite!" << endl;
			return 3;
		}
	}

	//-------------------------------------------------------------------------
	// Open input file and validate
	//-------------------------------------------------------------------------

	wmaReader = new CWmaReader();

	if(wmaReader->getRuntimeVersion(param.rtVersion, 128))
	{
		if(param.rtVersion)
		{
			fwprintf(stderr, L"WM Runtime:\tv%s\n", param.rtVersion);
		}
	}

	wcerr << "\nOpening input file... " << flush;

	if(!wmaReader->isValid(param.inputFile))
	{
		wcerr << "Failed\n\nSorry, this file is invalid/unsupported and can not be processed." << endl;
		wcerr << "Make sure that the supplied input file is a valid ASF (WMA/WMV) file!" << endl;
		SAFE_DELETE(wmaReader);
		return 4;
	}
	if(wmaReader->isProtected(param.inputFile))
	{
		wcerr << "Failed\n\nSorry, DRM infected ASF (WMA/WMV) files can not be processed." << endl;
		wcerr << "See `http://en.wikipedia.org/wiki/Windows_Media_DRM` for details!" << endl;
		SAFE_DELETE(wmaReader);
		return 5;
	}
	if(!wmaReader->open(param.inputFile))
	{
		wcerr << "Failed\n\nSorry, this file could not be opended although it appears to be valid." << endl;
		wcerr << "Make sure that the supplied input file is a valid ASF (WMA/WMV) file!" << endl;
		SAFE_DELETE(wmaReader);
		return 6;
	}
	
	//-------------------------------------------------------------------------
	// Analyze input & setup output format
	//-------------------------------------------------------------------------

	wcerr << "OK\nAnalyzing input file... " << flush;

	if(!wmaReader->analyze(&format))
	{
		wcerr << "Failed\n\nThis usually indicates that the ASF file contains no suitable audio stream." << endl;
		SAFE_DELETE(wmaReader);
		return 7;
	}
	
	if(!(param.defaultFormat))
	{
		wcerr << "OK\nConfiguring output format... " << flush;
		fix_format_pcm(&format);
		if(!(wmaReader->configureOutput(&format)))
		{
			wcerr << "Failed\nConfiguring output format... " << flush;
			LIMIT_TO(format.nChannels, 2);
			fix_format_pcm(&format);
			if(!(wmaReader->configureOutput(&format)))
			{
				wcerr << "Failed\nConfiguring output format... " << flush;
				LIMIT_TO(format.nSamplesPerSec, 48000);
				fix_format_pcm(&format);
				if(!(wmaReader->configureOutput(&format)))
				{
					wcerr << "Failed" << endl;
				}
				else
				{
					wcerr << "OK" << endl;
				}
			}
			else
			{
				wcerr << "OK" << endl;
			}
		}
		else
		{
			wcerr << "OK" << endl;
		}
	}
	else
	{
		wcerr << "OK" << endl;
	}
	
	//-------------------------------------------------------------------------
	// Detect output audio properties
	//-------------------------------------------------------------------------

	wcerr << "Detecting output format... " << flush;

	if(!wmaReader->getOutputFormat(&format))
	{
		wcerr << "Failed\n\nInternal Error: Could not determine output format." << endl;
		return 8;
	}

	wcerr << "OK\n\n[Audio Properties]" << endl;
	wcerr << "wFormatTag: " << hex << format.wFormatTag << dec << endl;
	wcerr << "nChannels: " << format.nChannels << endl;
	wcerr << "nSamplesPerSec: " << format.nSamplesPerSec << endl;
	wcerr << "wBitsPerSample: " << format.wBitsPerSample << endl;
	wcerr << "nAvgBytesPerSec: " << format.nAvgBytesPerSec << endl;
	wcerr << "nBlockAlign: " << format.nBlockAlign << endl;

	if((duration = wmaReader->getDuration()) > 0.0)
	{
		double duration_minutes, duration_seconds;
		seconds_to_minutes(duration, &duration_minutes, &duration_seconds);
		fwprintf(stderr, L"fDuration: %.0f:%04.1f\n", duration_minutes, duration_seconds);
		duration = min(duration, param.maxTime);
	}
	
	if(wmaReader->getCodecInfo(param.codecName, param.codecInfo, 128))
	{
		if(param.codecName)
		{
			fwprintf(stderr, L"sCodecName: %s\n", ltrim(param.codecName));
		}
		if(param.codecInfo)
		{
			fwprintf(stderr, L"sCodecInfo: %s\n", ltrim(param.codecInfo));
		}
	}

	if(wmaReader->getTitle(param.title, 128))
	{
		if(param.title)
		{
			fwprintf(stderr, L"sTitle: %s\n", ltrim(param.title));
		}
	}

	if((bufferLen = wmaReader->getSampleSize()) < 1)
	{
		wcerr << "\nFailed to detect maximum sample size!" << endl;
		SAFE_DELETE(wmaReader);
		return 9;
	}
	
	wcerr << "nMaxSampleSize: " << bufferLen << endl;
	wcerr << "\nOpening output file... " << flush;
	
	//-------------------------------------------------------------------------
	// Open output file for writing
	//-------------------------------------------------------------------------

	sink = (!(param.rawOutput)) ? dynamic_cast<CAbstractSink*>(new CWaveWriter()) : dynamic_cast<CAbstractSink*>(new CRawWriter());

	if(!sink->open(param.outputFile, &format))
	{
		wcerr << "Failed" << endl;
		SAFE_DELETE(wmaReader);
		return 10;
	}

	wcerr << "OK\n" << endl;

	if(param.silentMode)
	{
		wcerr << "Dumping audio samples to file, please be patient..." << flush;
	}

	//-------------------------------------------------------------------------
	// Main processing loop (dump audio)
	//-------------------------------------------------------------------------
	
	g_aborted = false;

	bufferLen = ((bufferLen / 4096) + 1) * 4096;
	buffer = new BYTE[bufferLen];
	SecureZeroMemory(buffer, bufferLen);

	while(!g_aborted)
	{
		if((!indicator) && (!(param.silentMode)))
		{
			if(duration > 0.0)
			{
				double completed = min((currentTime / duration) * 100.0, 100.0);
				double currentTime_minutes, currentTime_seconds, duration_minutes, duration_seconds;
				seconds_to_minutes(currentTime, &currentTime_minutes, &currentTime_seconds);
				seconds_to_minutes(duration, &duration_minutes, &duration_seconds);
				fwprintf(stderr, L"\r[%3.1f%%] %.0f:%04.1f of %.0f:%04.1f completed, please wait... %C", completed, currentTime_minutes, currentTime_seconds, duration_minutes, duration_seconds, alive[aliveIndex]);
			}
			else
			{
				double currentTime_minutes, currentTime_seconds;
				seconds_to_minutes(currentTime, &currentTime_minutes, &currentTime_seconds);
				fwprintf(stderr, L"\r%.0f:%04.1f seconds completed so far... %C", currentTime_minutes, currentTime_seconds, alive[aliveIndex]);
			}
			
			aliveIndex = (aliveIndex + 1) % 4;
			fflush(stderr);
		}
		
		indicator = (indicator + 1) % 50;
		
		double sampleDuration = -1.0;
		double sampleTimestamp = -1.0;
		size_t skipBytes = 0;

		if(!wmaReader->getNextSample(buffer, bufferLen, &sampleLen, &sampleTimestamp, &sampleDuration))
		{
			wcerr << "\n\nFailed to read sample from input file!" << endl;
			SAFE_DELETE(sink);
			SAFE_DELETE(wmaReader);
			SAFE_DELETE_ARRAY(buffer);
			return 11;
		}
		
		if(!((sampleLen > 0) && (currentTime < param.maxTime)))
		{
			if(!(param.silentMode))
			{
				double currentTime_minutes, currentTime_seconds;
				seconds_to_minutes(currentTime, &currentTime_minutes, &currentTime_seconds);
				fwprintf(stderr, L"\r[%3.1f%%] %.0f:%04.1f of %.0f:%04.1f completed, please wait... %C", 100.0, currentTime_minutes, currentTime_seconds, currentTime_minutes, currentTime_seconds, 0x20);
				fflush(stderr);
			}
			break;
		}

		if((sampleLen % format.nBlockAlign) > 0)
		{
			fwprintf(stderr, L"\rInconsistent sample length: %I64u is not a multiple of %I64u.\n", static_cast<unsigned __int64>(sampleLen), static_cast<unsigned __int64>(format.nBlockAlign));
		}
		
		if((sampleTimestamp >= 0.0) && (abs(sampleTimestamp - currentTime) > maxGapSize))
		{
			if(!(param.silentMode))
			{
				fwprintf(stderr, L"\rInconsistent timestamps: Expected %10.8f next, but got %10.8f.\n", currentTime, sampleTimestamp);
			}

			if(sampleTimestamp > currentTime)
			{
				size_t paddingBytes = time_to_bytes((sampleTimestamp - currentTime), &format);
				
				if(paddingBytes > 0)
				{
					BYTE zeroBuffer[1024];
					SecureZeroMemory(zeroBuffer, 1024);

					if(!(param.silentMode))
					{
						fwprintf(stderr, L"There is a \"gap\" of %10.8f seconds, padding %I64u zero bytes!\n", (sampleTimestamp - currentTime), static_cast<unsigned __int64>(paddingBytes));
					}

					currentTime += bytes_to_time(paddingBytes, &format);
					stats[0] += paddingBytes;

					while(paddingBytes > 0)
					{
						size_t currentSize = min(paddingBytes, 1024);
						if(!sink->write(currentSize, zeroBuffer))
						{
							wcerr << "\n\nFailed to write sample to output file!" << endl;
							SAFE_DELETE(sink);
							SAFE_DELETE(wmaReader);
							SAFE_DELETE_ARRAY(buffer);
							return 12;
						}
						paddingBytes -= currentSize;
					}
				}
			}
			else
			{
				size_t offsetBytes = time_to_bytes((currentTime - sampleTimestamp), &format);
				
				if(offsetBytes > 0)
				{
					skipBytes = min(offsetBytes, sampleLen);
					stats[1] += skipBytes;

					if(!(param.silentMode))
					{
						fwprintf(stderr, L"The samples \"overlap\" for %10.8f seconds, skipping %I64u bytes!\n", (currentTime - sampleTimestamp), static_cast<unsigned __int64>(skipBytes));
					}

					currentTime -= bytes_to_time(skipBytes, &format);
				}
			}
		}

		if(!sink->write((sampleLen - skipBytes), (buffer + skipBytes)))
		{
			wcerr << "\n\nFailed to write sample to output file!" << endl;
			SAFE_DELETE(sink);
			SAFE_DELETE(wmaReader);
			SAFE_DELETE_ARRAY(buffer);
			return 13;
		}

		if((sampleDuration >= 0.0) && (!(param.alternativeMode)))
		{
			if(sampleTimestamp >= 0.0)
			{
				currentTime = sampleTimestamp + sampleDuration;
			}
			else 
			{
				currentTime += bytes_to_time(sampleLen, &format);
			}
		}
		else
		{
			currentTime += bytes_to_time(sampleLen, &format);
		}
	}

	//-------------------------------------------------------------------------
	// Close output and shutdown
	//-------------------------------------------------------------------------

	if(!sink->close())
	{
		wcerr << "\n\nError: Failed to properly close the output file!" << endl;
		SAFE_DELETE(wmaReader);
		SAFE_DELETE(sink);
		SAFE_DELETE_ARRAY(buffer);
		return 14;
	}

	if((stats[0] > 0) || (stats[1] > 0))
	{
		wcerr << "\nWarning: Sync correction inserted " << stats[0] << " zero bytes, skipped " << stats[1] << " bytes." << flush;
	}

	SAFE_DELETE(wmaReader);
	SAFE_DELETE(sink);
	SAFE_DELETE_ARRAY(buffer);

	if(g_aborted)
	{
		wcerr << "\n\nOperation aborted by user!" << endl;
		return 15;
	}
	else
	{
		wcerr << "\n\nAll done." << endl;
		return 0;
	}
}
