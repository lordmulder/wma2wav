@echo off
rem -------------------------------------------------------------------------
rem Options
rem -------------------------------------------------------------------------
set "PATH_MSVC=D:\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
set "PATH_7ZIP=E:\7-Zip"
set "PATH_UPX=E:\MPUI\Installer"
set "PATH_MPRESS=E:\MPress"
rem
rem -------------------------------------------------------------------------
rem DO NOT MODIFY ANY LINES BELOW THIS LINE !!!
rem -------------------------------------------------------------------------
set "PATH_SOLUTION=%~dp0\wma2wav.sln"
set "PATH_RELEASE=%~dp0\Release"
set "PATH_TEMP=%TMP%\~%RANDOM%%RANDOM%.tmp"
set "DATE_TEMP_FILE=%PATH_TEMP%\date.tag"
rem
rem -------------------------------------------------------------------------
if not exist "%~dp0\etc\date.exe" (
  echo Error: %~dp0\etc\date.exe not found.
  goto BUILD_DONE
)
set "DATE_TEMP_FILE=%TEMP%\~date.%RANDOM%%RANDOM%.tmp"
"%~dp0\etc\date.exe" +%%Y-%%m-%%d > "%DATE_TEMP_FILE%"
set /p "ISO_DATE=" < "%DATE_TEMP_FILE%"
rem
rem -------------------------------------------------------------------------
call "%PATH_MSVC%" x86
msbuild.exe /property:Configuration=Release /property:Platform=Win32 /target:Clean /verbosity:detailed "%PATH_SOLUTION%"
msbuild.exe /property:Configuration=Release /property:Platform=Win32 /target:Rebuild /verbosity:detailed "%PATH_SOLUTION%"
msbuild.exe /property:Configuration=Release /property:Platform=Win32 /target:Build /verbosity:detailed "%PATH_SOLUTION%"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
call "%PATH_MSVC%" amd64
msbuild.exe /property:Configuration=Release /property:Platform=x64 /target:Clean /verbosity:detailed "%PATH_SOLUTION%"
msbuild.exe /property:Configuration=Release /property:Platform=x64 /target:Rebuild /verbosity:detailed "%PATH_SOLUTION%"
msbuild.exe /property:Configuration=Release /property:Platform=x64 /target:Build /verbosity:detailed "%PATH_SOLUTION%"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
mkdir "%PATH_TEMP%"
copy "%PATH_RELEASE%\Win32\wma2wav.exe" "%PATH_TEMP%\wma2wav.exe"
copy "%PATH_RELEASE%\x64\wma2wav.exe" "%PATH_TEMP%\wma2wav-x64.exe"
copy "%~dp0\*.txt" "%PATH_TEMP%"
rem
rem -------------------------------------------------------------------------
"%PATH_7ZIP%\7z.exe" a -ttar "%PATH_TEMP%\wma2wav.tar" "@%~dp0\Build.src"
"%PATH_UPX%\upx.exe" --brute "%PATH_TEMP%\wma2wav.exe"
"%PATH_MPRESS%\mpress.exe" -s "%PATH_TEMP%\wma2wav-x64.exe"
rem
rem -------------------------------------------------------------------------
del /F "%~dp0\wma2wav.%ISO_DATE%.zip"
"%PATH_7ZIP%\7z.exe" a -tzip "%~dp0\wma2wav.%ISO_DATE%.zip" "%PATH_TEMP%\*.*"
if not "%ERRORLEVEL%"=="0" goto BUILD_DONE
rem
rem -------------------------------------------------------------------------
rmdir /S /Q "%PATH_TEMP%"
rem
rem -------------------------------------------------------------------------
:BUILD_DONE
pause
