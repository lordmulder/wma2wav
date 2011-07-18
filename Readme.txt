wma2wav - Dump WMA/WMV files to Wave Audio [Jul 18 2011]
Copyright (c) 2011 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
Released under the terms of the GNU General Public License.

http://code.google.com/p/mulder/


Usage Instructions
------------------

Usage:
  wma2wav.exe [options] -i <input> -o <output>

Options:
  -i <input>   Select input ASF (WMA/WMV) file to read from
  -o <output>  Select output Wave file to write to, specify "-" for STDOUT
  -f           Force overwrite of output file (if already exists)
  -r           Output "raw" PCM data to file instead of Wave/RIFF file
  -s           Silent mode, do not display progress indicator
  -x           Use the "alternative" timestamp calculation mode
  -a           Enable "aggressive" sync correction mode (not recommended)
  -n           No sync correction (can not use with '-a' or '-x')

Example:
  wma2wav.exe "c:\my music\input.wma" "c:\temp\output.wav"


Prerequisites
-------------

This tool requires the Windows Media Format Runtime. Version 9 or later should work, Version 11 is recommended.

The Windows Media Format Runtime is included with Windows as part of Windows Media Player. So you should normally only need to install the Format Runtime if you have removed Windows Media Player or if you are using the special European 'N' or Korean 'KN' edition of Windows that does not include WMP. It also is required to install the Windows Media Format Runtime if you are on Wine/Linux.

For more info and Windows Media Format Runtime downloads, please look at this web-site:
http://www.codecguide.com/windows_media_codecs.htm


OS Support
----------

The pre-compiled binaries of this tool were compiled with Visual Studio 2010 and therefore will only work on Windows XP with Service Pack 2 or any later version of Microsoft Windows (including Vista and Windows 7). It does NOT run on Windows 2000 though! Please compile with Visual Studio 2008, if you still need Windows 2000 support. The tool has been tested to work with Linux (Ubuntu 11.04) thanks to Wine.
