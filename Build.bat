@echo off
rem -------------------------------------------------------------------------
rem Options
rem -------------------------------------------------------------------------
set "PATH_MSVC=D:\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
set "PATH_7ZIP=E:\7-Zip"
set "PATH_UPX=E:\MPUI\Installer"
set "PATH_MPRESS=E:\MPress"
set "MSBUILD_VERBOSITY=normal"
rem
rem -------------------------------------------------------------------------
rem DO NOT MODIFY ANY LINES BELOW THIS LINE !!!
rem -------------------------------------------------------------------------
set "PATH_SOLUTION=%~dp0\wma2wav.sln"
set "PATH_RELEASE=%~dp0\Release"
set "PATH_DEBUG=%~dp0\Debug"
set "PATH_TEMP=%TMP%\~%RANDOM%%RANDOM%.tmp"
set "DATE_TEMP_FILE=%PATH_TEMP%\build_date.tag"
rem
rem -------------------------------------------------------------------------
if not exist "%~dp0\etc\date.exe" (
  echo Error: %~dp0\etc\date.exe not found.
  goto BUILD_DONE
)
mkdir "%PATH_TEMP%"
"%~dp0\etc\date.exe" +%%Y-%%m-%%d > "%DATE_TEMP_FILE%"
set /p "ISO_DATE=" < "%DATE_TEMP_FILE%"
rem
rem -------------------------------------------------------------------------
call "%PATH_MSVC%" x86
rem
rem -------------------------------------------------------------------------
set "MSBUILD_PARAMS=/property:Configuration=Release /property:Platform=Win32 /verbosity:%MSBUILD_VERBOSITY%"
msbuild.exe %MSBUILD_PARAMS% /target:Clean "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Rebuild "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Build "%PATH_SOLUTION%"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
set "MSBUILD_PARAMS=/property:Configuration=Debug /property:Platform=Win32 /verbosity:%MSBUILD_VERBOSITY%"
msbuild.exe %MSBUILD_PARAMS% /target:Clean "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Rebuild "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Build "%PATH_SOLUTION%"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
call "%PATH_MSVC%" amd64
rem
rem -------------------------------------------------------------------------
set "MSBUILD_PARAMS=/property:Configuration=Release /property:Platform=x64 /verbosity:%MSBUILD_VERBOSITY%"
msbuild.exe %MSBUILD_PARAMS% /target:Clean "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Rebuild "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Build "%PATH_SOLUTION%"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
set "MSBUILD_PARAMS=/property:Configuration=Debug /property:Platform=x64 /verbosity:%MSBUILD_VERBOSITY%"
msbuild.exe %MSBUILD_PARAMS% /target:Clean "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Rebuild "%PATH_SOLUTION%"
msbuild.exe %MSBUILD_PARAMS% /target:Build "%PATH_SOLUTION%"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
copy "%PATH_RELEASE%\Win32\wma2wav.exe" "%PATH_TEMP%\wma2wav.exe"
copy "%PATH_DEBUG%\Win32\wma2wav.exe" "%PATH_TEMP%\wma2wav-dbg.exe"
copy "%PATH_RELEASE%\x64\wma2wav.exe" "%PATH_TEMP%\wma2wav-x64.exe"
copy "%~dp0\*.txt" "%PATH_TEMP%"
rem
rem -------------------------------------------------------------------------
"%PATH_7ZIP%\7z.exe" a -ttar "%PATH_TEMP%\wma2wav.tar" "@%~dp0\Build.src"
"%PATH_UPX%\upx.exe" --brute "%PATH_TEMP%\wma2wav.exe"
"%PATH_MPRESS%\mpress.exe" -s "%PATH_TEMP%\wma2wav-x64.exe"
rem
rem -------------------------------------------------------------------------
set "OUTFILENAME=%~dp0\wma2wav.%ISO_DATE%.zip"
if exist "%OUTFILENAME%" set "OUTFILENAME=%~dp0\wma2wav.%ISO_DATE%.V%RANDOM%.zip"
"%PATH_7ZIP%\7z.exe" a -tzip "%OUTFILENAME%" "%PATH_TEMP%\*.*"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
attrib +R "%OUTFILENAME%"
rmdir /S /Q "%PATH_TEMP%"
rem
rem -------------------------------------------------------------------------
:BUILD_DONE
pause
