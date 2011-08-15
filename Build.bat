@echo off
rem -------------------------------------------------------------------------
rem Options
rem -------------------------------------------------------------------------
set "PATH_MSVC=C:\Program Files\Microsoft Visual Studio 10.0"
set "PATH_7ZIP=C:\Program Files\7-Zip" 
set "PATH_UPX=C:\Program Files\UPX"
set "PATH_MPRESS=C:\Program Files\MPress"
set "MSBUILD_VERBOSITY=normal"
set "MSVC9_INSTALLED=0"
rem -------------------------------------------------------------------------
rem DO NOT MODIFY ANY LINES BELOW THIS LINE !!!
rem -------------------------------------------------------------------------
if "%~4"=="" goto BUILD_BEGIN
rem -------------------------------------------------------------------------
title MSBuild - %2 %3 %4
call "%PATH_MSVC%\VC\vcvarsall.bat" %~2
set "MSBUILD_PARAMS=/property:Configuration=%~4 /property:Platform=%~3 /verbosity:%MSBUILD_VERBOSITY%"
msbuild.exe %MSBUILD_PARAMS% /target:Clean "%~1"
msbuild.exe %MSBUILD_PARAMS% /target:Rebuild "%~1"
msbuild.exe %MSBUILD_PARAMS% /target:Build "%~1"
if %ERRORLEVEL% neq 0 pause
exit %ERRORLEVEL%
rem -------------------------------------------------------------------------
:BUILD_BEGIN
echo Build in progress, please wait...
echo.
rem -------------------------------------------------------------------------
set "PATH_SOLUTION=%~dp0\wma2wav.sln"
set "PATH_RELEASE=%~dp0\Release"
set "PATH_RELEASE_VC9=%~dp0\Release_VC9"
set "PATH_DEBUG=%~dp0\Debug"
set "PATH_TEMP=%TMP%\~%RANDOM%%RANDOM%.tmp"
set "DATE_TEMP_FILE=%PATH_TEMP%\build_date.tag"
rem -------------------------------------------------------------------------
if not exist "%~dp0\etc\date.exe" (
  echo Error: %~dp0\etc\date.exe not found!
  goto BUILD_DONE
)
if not exist "%PATH_MSVC%\VC\vcvarsall.bat" (
  echo Error: %PATH_MSVC%\VC\vcvarsall.bat not found!
  goto BUILD_DONE
)
if not exist "%PATH_7ZIP%\7z.exe" (
  echo Error: %PATH_7ZIP%\7z.exe not found!
  goto BUILD_DONE
)
rem -------------------------------------------------------------------------
mkdir "%PATH_TEMP%"
"%~dp0\etc\date.exe" +%%Y-%%m-%%d > "%DATE_TEMP_FILE%"
set /p "ISO_DATE=" < "%DATE_TEMP_FILE%"
rem -------------------------------------------------------------------------
start "" /WAIT cmd.exe /c ""%~dpnx0" "%PATH_SOLUTION%" x86 Win32 Release"
if %ERRORLEVEL% neq 0 goto BUILD_DONE
start "" /WAIT cmd.exe /c ""%~dpnx0" "%PATH_SOLUTION%" x86 Win32 Debug"
if %ERRORLEVEL% neq 0 goto BUILD_DONE
start "" /WAIT cmd.exe /c ""%~dpnx0" "%PATH_SOLUTION%" x86_amd64 x64 Release"
if %ERRORLEVEL% neq 0 goto BUILD_DONE
start "" /WAIT cmd.exe /c ""%~dpnx0" "%PATH_SOLUTION%" x86_amd64 x64 Debug"
if %ERRORLEVEL% neq 0 goto BUILD_DONE
rem -------------------------------------------------------------------------
if %MSVC9_INSTALLED% neq 0 (
  start "" /WAIT cmd.exe /c ""%~dpnx0" "%PATH_SOLUTION%" x86 Win32 Release_VC9"
)
if %ERRORLEVEL% neq 0 goto BUILD_DONE
rem -------------------------------------------------------------------------
copy "%PATH_RELEASE%\Win32\wma2wav.exe" "%PATH_TEMP%\wma2wav.exe"
copy "%PATH_DEBUG%\Win32\wma2wav.exe" "%PATH_TEMP%\wma2wav-dbg.exe"
copy "%PATH_RELEASE%\x64\wma2wav.exe" "%PATH_TEMP%\wma2wav-x64.exe"
copy "%~dp0\*.txt" "%PATH_TEMP%"
rem -------------------------------------------------------------------------
"%PATH_7ZIP%\7z.exe" a -ttar "%PATH_TEMP%\wma2wav.tar" "@%~dp0\Build.src"
"%PATH_UPX%\upx.exe" --brute "%PATH_TEMP%\wma2wav.exe"
"%PATH_MPRESS%\mpress.exe" -s "%PATH_TEMP%\wma2wav-x64.exe"
rem -------------------------------------------------------------------------
if %MSVC9_INSTALLED% neq 0 (
  copy "%PATH_RELEASE_VC9%\Win32\wma2wav.exe" "%PATH_TEMP%\wma2wav-msvc9.exe"
  "%PATH_UPX%\upx.exe" --brute "%PATH_TEMP%\wma2wav-msvc9.exe"
)
rem -------------------------------------------------------------------------
set "OUTFILENAME=%~dp0\wma2wav.%ISO_DATE%.zip"
if exist "%OUTFILENAME%" set "OUTFILENAME=%~dp0\wma2wav.%ISO_DATE%.V%RANDOM%.zip"
"%PATH_7ZIP%\7z.exe" a -tzip "%OUTFILENAME%" "%PATH_TEMP%\*.*"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem -------------------------------------------------------------------------
attrib +R "%OUTFILENAME%"
rmdir /S /Q "%PATH_TEMP%"
rem
rem -------------------------------------------------------------------------
:BUILD_DONE
pause
