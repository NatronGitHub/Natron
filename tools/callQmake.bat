@echo off
setlocal enabledelayedexpansion
if "%1"=="64" (
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\vcvars64.bat"
) else (
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"
)

qmake -r -tp vc -spec C:\Qt\4.8.6_win32\mkspecs\win32-msvc2010 CONFIG+=%1bit
