wma2wav - Dump WMA/WMV files to uncompressed Wave Audio
Copyright (c) 2011 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
http://code.google.com/p/mulder/

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License <http://www.gnu.org/>.
Note that this program is distributed with ABSOLUTELY NO WARRANTY.


Introduction
------------

wma2wav is a simple command-line tool for decoding/converting Windows Media Audio (WMA) streams to uncompressed Wave Audio files or to "raw" PCM data. It supports all WMA versions up to 10.0, including but not limited to the "Professional", "Lossless" and "Voice" variants. All ASF (Advanced Systems Format) files containing WMA audio, including ".WMA" and ".WMV" files, can be processed as input. Note, however, that this tool does *not* circumvent any DRM (Digital Rights Management) restrictions, i.e. encrypted/protected files can *not* be processed.


Usage Instructions
------------------

Usage:
  wma2wav.exe [options] -i <input> -o <output>

Options:
  -i <input>   Select input ASF (WMA/WMV) file to read from
  -o <output>  Select output Wave file to write to, specify "-" for STDOUT
  -t <time>    Maximum number of seconds to dump (will stop at maximum)
  -f           Force overwrite of output file, if file already exists
  -r           Output "raw" PCM data to file instead of Wave/RIFF file
  -s           Silent mode, do not display progress indicator
  -x           Use the "alternative" timestamp calculation mode
  -a           Enable "aggressive" sync correction mode (not recommended)
  -n           No sync correction (can not use with '-a' or '-x')
  -d           Use "default" audio output format (e.g. Stereo, 16-Bit)

Example:
  wma2wav.exe -i "c:\my music\input.wma" -o "c:\my music\output.wav"


Prerequisites
-------------

This tool uses the Windows Media Format Runtime, so the Windows Media Format Runtime must be installed on the computer. Version 9 of the Windows Media Format Runtime should be sufficient, but version 11 or later is recommended.

The Windows Media Format Runtime is included with Windows as part of the Windows Media Player (WMP). So you should only need to download and install the Windows Media Format Runtime separately, if you have removed Windows Media Player or if you are using the special European 'N' or Korean 'KN' edition of Windows that does NOT include Windows Media Player. Moreover it may be required to download and install the Windows Media Format Runtime separately, if you are running this tool under Wine/Linux.

For more information about the Windows Media Format Runtime, please visit the following web-site:
http://www.codecguide.com/windows_media_codecs.htm


OS Support
----------

The pre-compiled binaries of this tool were compiled with Visual Studio 2010 (MSVC 10.0) and therefore will only work on Windows XP with Service Pack 2 or any later version of Microsoft Windows (including Vista and Windows 7). They do NOT run on Windows 2000 though! There is a special Visual Studio 2008 (MSVC 9.0) binary included, which should run on Windows 2000. Moreover there is a "x64" (64-Bit) binary that will run on the 64-Bit editions of Vista and Windows 7 only. Last but not least this tool has been tested to work under Linux (Ubuntu 11.04), thanks to Wine.

Note: The "debug" binary is intended for testing/debugging purposes only and should NOT be used for normal operation.


Sync Correction
---------------

This tool implements "sync correction" for ASF (WMA/WMV) files. Sync correction is required for keeping the audio/video in sync, i.e. when dumping the audio part of a WMV file, because in a WMA/WMV stream each individual so-called "sample" (actually a chunk of audio samples) has its own timestamp/duration attached. Generally it is assumed that the timestamps of the audio stream are consistent and continuous. So if timestamp[i] denotes the timestamp of the i-th sample and duration[i] denotes the duration of the i-th sample, then "timestamp[n] = timestamp[n-1] + duration[n-1]" shall be true for all samples n. If, however, this equation does NOT hold true for a certain sample n, then we have a discontinuity - either there is a "gap" between the audio samples or the audio samples "overlap". This tool tries to compensate for "gaps" by padding with zero bytes and to compensate for "overlaps" by skipping bytes. As there always are tiny discontinuities (probably due to limited timestamp precision), this tool will only compensate for gaps/overlaps that exceed a certain threshold by default. This way excessive padding is avoided. The "aggressive" mode will compensate for ALL gaps/overlaps, which in theory should be more accurate, but in reality doesn't seem to work very well. There also is an "alternative" timestamp calculation mode available. While the default mode will calculate the expected(!) timestamp of the NEXT sample as the sum of the timestamp & duration of the CURRENT sample, the alternative mode calculates the expected(!) timestamp of the NEXT sample as the accumulated sum of the measured(!) durations of ALL samples processed so far. If you are just processing audio-only files, you do not need sync correction and you may disable it entirely.

If anybody knows of a better method to sync WMA streams, please drop me a note ;-)


Output Formats
--------------

The WMA decoder supports multiple output formats. By default it uses something like 16-Bit and 2 channels (Stereo), even for 24-Bit or multi-channels sources. This tool tries to configure an output format that is identical to the source audio's "native" format, which should allow 24-Bit and/or multi-channel output (for suitable sources). It has to be noted that unlocking "true" multi-channel output from the WMA decoder requires additional steps and is supported on Windows XP (and later) only. There also is an option to output the audio using the "default" output format. You can use that option, if "high" bit-depth or multi-channel output is NOT desired.


Changelog
---------

[2011-07-22] Fixed a bug which could cause the application to crash, when the Title, Codec Name or Codec Info of an ASF file exceeded a length of 128 characters.
[2011-07-24] Workaround for a bug in LoadLibraryEx() that could cause loading WMVCORE.DLL to fail or even make the application crash on certain systems (e.g. 64-Bit Vista without KB2533623)


eof.
