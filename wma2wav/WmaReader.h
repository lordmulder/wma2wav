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

#pragma once

#include "stdafx.h"
struct IWMSyncReader;
struct IWMStreamConfig;

class CWmaReader
{
public:
	CWmaReader(void);
	~CWmaReader(void);

	/* getRuntimeVersion()
	/*   This function can be used to detect the version of the Windows Media Runtime that is used.
	/*   The 'version' parameter points to a pre-allocated buffer that will receive the version string. The 'size' parameter specifies the size of the buffer, in wchar_t's.
	/*   Returns TRUE if successfull, FALSE otherwise.
	 */
	bool getRuntimeVersion(wchar_t *version, size_t size);

	/* isValid()
	/*   This function checks wether a given file is a valid ASF file that can be processed. Only valid files can be processed!
	/*   The 'filename' parameter points to a null-terminated string that specifies the path to the file that should be checked.
	/*   Returns TRUE if the specified file is readable and valid, FALSE otherwise.
	 */
	bool isValid(const wchar_t *filename);

	/* isProtected()
	/*   This function checks wether a given ASF file is "protected" by DRM. If file is protected, it cannot be processed!
	/*   The 'filename' parameter points to a null-terminated string that specifies the path to the file that should be checked.
	/*   Returns TRUE if the specified file is a protected ASF file, FALSE otherwise.
	 */
	bool isProtected(const wchar_t *filename);

	/* open()
	/*   This function opens the given ASF file for reading. Can only succeed, if no file is not open yet or the previous file was closed!
	/*   The 'filename' parameter points to a null-terminated string that specifies the path to the file that should be opened. Only valid and unprotected ASF files can be opened.
	/*   Returns TRUE if the specified file was opened successfully, FALSE otherwise.
	 */
	bool open(const wchar_t *filename);

	/* close()
	/*   This function closes the file that is currently open. It does nothing, if no file is currently open.
	 */
	void close(void);

	/* analyze()
	/*   This function analyses the input ASF file and tries to find a suitable audio stream.
	/*   The 'format' parameter points to a pre-allocated WAVEFORMATEX structure that will receive the "native" format of the audio stream - only valid if the function succeeded.
	/*   Note that the format specification returned by this function generally is NOT identical to the format of the audio samples returned by the getNextSample() function!
	/*   Returns TRUE if the analysis completed successfully and an audio stream was found, FALSE otherwise.
	 */
	bool analyze(WAVEFORMATEX *format);
	
	/* configureOutput()
	/*   This function changes the output format, i.e. the format of the samples returned by getNextSample().
	/*   The 'format' parameter points to a pre-allocated WAVEFORMATEX structure that contains the new output format specification.
	/*   Returns TRUE if the output format was changed successfully, FALSE otherwise.
	/*   If the function succeeded, call getOutputFormat() afterwards in order to obtain the new output format!
	 */
	bool configureOutput(WAVEFORMATEX *format);
	
	/* getOutputFormat()
	/*   This function determines the current output format, i.e. the format of the samples returned by getNextSample().
	/*   The 'format' parameter points to a pre-allocated WAVEFORMATEX structure that will receive the output format of the audio stream - only valid if the function succeeded.
	/*   Returns TRUE if the output format was read successfully, FALSE otherwise.
	 */
	bool getOutputFormat(WAVEFORMATEX *format);
	
	/* getSampleSize()
	/*   This function determines the maximum sample size. The size of the output buffer passed to getNextSample() must have at least this size!
	/*   Returns the maximum sample size, in bytes. If the funcation failed, it returns ZERO.
	 */
	size_t getSampleSize(void);
	
	/* getDuration()
	/*   This function determines the duration of the media that is currently open.
	/*   Returns the duration, in seconds. If the funcation failed, the return value is nagative.
	 */
	double getDuration(void);
	
	/* getCodecInfo()
	/*   This function determines the Codec name and the Codec info of the audio stream, if that information is available.
	/*   The 'codecName' parameter points to a pre-allocated buffer that will receive the Codec name. The size parameter specifies the buffer size, in wchar_t's.
	/*   The 'codecInfo' parameter points to a pre-allocated buffer that will receive the Codec name. The size parameter specifies the buffer size, in wchar_t's.
	/*   Returns TRUE if the information was read successfully, FALSE otherwise.
	 */
	bool getCodecInfo(wchar_t *codecName, wchar_t *codecInfo, size_t size);
	
	/* getTitle()
	/*   This function determines the title of the that is currently open, if that information is available.
	/*   The 'title' parameter points to a pre-allocated buffer that will receive the title of the media. The size parameter specifies the buffer size, in wchar_t's.
	/*   Returns TRUE if the title was read successfully, FALSE otherwise.
	 */
	bool getTitle(wchar_t *title, size_t size);
	
	/* getNextSample()
	/*   This function reads the next chunk of audio samples from the input audio stream.
	/*   The 'output' parameter points to a pre-allocated buffer that will received the audio samples. The 'size' parameter specifies the size of the output buffer, in bytes.
	/*   The 'length' parameter points to a variable the will receive the number of bytes that were written into the output buffer. ZERO indicates the end of the stream.
	/*   The optional 'timeStamp' and 'sampleDuration' parameters point to variables that will receive the timestamp and the duration, in seconds, of the samples that were read.
	/*   The required minimum output buffer size, in bytes, can be determined by calling the getSampleSize() function. If the output buffer is smaller, samples may be dropped!
	/*   The format specification of the audio samples returned by this function can be determined by calling the getOutputFormat() function.
	/*   Returns TRUE if the samples were read successfully (even if the end of the stream was reached), FALSE otherwise.
	 */
	bool getNextSample(BYTE *output, const size_t size, size_t *length, double *timeStamp = NULL, double *sampleDuration = NULL);

private:
	bool m_isOpen;
	bool m_isAnalyzed;

	HMODULE m_wmvCore;
	DWORD m_wmvCoreVersion[4];
	IWMSyncReader *m_reader;
	DWORD m_outputNum;
	WORD m_streamNum;
	
	bool _findAudioStream(WAVEFORMATEX *format);
	bool _testAudioStream(WAVEFORMATEX *format, const WORD idx, IWMStreamConfig *pIWMStreamConfig);
};
