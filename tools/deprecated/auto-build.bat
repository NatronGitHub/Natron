:: ***** BEGIN LICENSE BLOCK *****
:: This file is part of Natron <http://www.natron.fr/>,
:: Copyright (C) 2016 INRIA and Alexandre Gauthier
::
:: Natron is free software: you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: Natron is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
:: ***** END LICENSE BLOCK *****

@echo off
set CWD=%cd%
set TMP=%CWD%\autobuild
set GIT_NATRON=https://github.com/MrKepzie/Natron.git
set GIT_IO=https://github.com/MrKepzie/openfx-io.git
set GIT_MISC=https://github.com/devernay/openfx-misc.git
set NATRON_BRANCH=workshop
set OPENFX_BRANCH=master
setlocal enabledelayedexpansion 


if not exist "%TMP%" (
	mkdir %TMP%
)

if not exist "%TMP%\Natron" (
	cd %TMP%
	git clone %GIT_NATRON%
	cd Natron
	git checkout %NATRON_BRANCH%
)


if not exist "%TMP%\openfx-io" (
	cd %TMP%
	git clone %GIT_IO%
)

if not exist "%TMP%\openfx-misc" (
	cd %TMP%
	git clone %GIT_MISC%
)

:MAIN_LOOP
set CONTINUE=0
echo Running...
set BUILD_ALL=0
set BUILD_NATRON=0
cd %TMP%\Natron
git pull origin %NATRON_BRANCH%
git submodule update -i --recursive
git log|head -1|awk "{print $2}" > curVersion.txt
set /p GITV_NATRON=<curVersion.txt
set /p ORIG_NATRON=<%CWD%\NATRON_WORKSHOP
echo Natron git %GITV_NATRON% vs. last version %ORIG_NATRON%
ECHO.%ORIG_NATRON% | FIND /I "%GITV_NATRON%">Nul && (
	echo "Natron up to date"
) || (
	echo "Natron update needed"
	set BUILD_NATRON=1
	set BUILD_ALL=1
	echo %GITV_NATRON% > %CWD%\NATRON_WORKSHOP
)

set BUILD_IO=0
cd %TMP%\openfx-io
git pull origin %OPENFX_BRANCH%
git submodule update -i --recursive
git log|head -1|awk "{print $2}" > curVersion.txt
set /p GITV_IO=<curVersion.txt
set /p ORIG_IO=<%CWD%\IO_WORKSHOP
echo openfx-io git %GITV_IO% vs. last version %ORIG_IO%
ECHO.%ORIG_IO% | FIND /I "%GITV_IO%">Nul && (
	echo "openfx-io up to date"
) || (
	echo "openfx-io update needed"
	set BUILD_IO=1
	set BUILD_ALL=1
	echo %GITV_IO% > %CWD%\IO_WORKSHOP
)

set BUILD_MISC=0
cd %TMP%\openfx-misc
git pull origin %OPENFX_BRANCH%
git submodule update -i --recursive
git log|head -1|awk "{print $2}" > curVersion.txt
set /p GITV_MISC=<curVersion.txt
set /p ORIG_MISC=<%CWD%\MISC_WORKSHOP
echo openfx-misc git %GITV_MISC% vs. last version %ORIG_MISC%
ECHO.%ORIG_MISC% | FIND /I "%GITV_MISC%">Nul && (
	echo "openfx-misc up to date"
) || (
	echo "openfx-misc update needed"
	set BUILD_MISC=1
	set BUILD_ALL=1
	echo %GITV_MISC% > %CWD%\MISC_WORKSHOP
)

if "%BUILD_ALL%"=="1" (
for /l %%i in (1, 1, 2) do (
    if "%%i" == "1" (
		set BIT=64
    )
	if "%%i" == "2" (
		set BIT=32
	)
	cd %CWD%
	set errorlevel=0
	call build-natron.bat %NATRON_BRANCH% !BIT! 0 Release 1
	if "%errorlevel%" == "1" (
		echo BUILD FAILED
		goto main_loop
	)
	cd %CWD%
	set errorlevel=0
	call build-plugins.bat !BIT! Release 1
	if "%errorlevel%" == "1" (
		echo BUILD FAILED
		goto main_loop
	)
	if exist "%TMP%\repo\Windows!BIT!" (
		rmdir /S /Q %TMP%\repo\Windows!BIT!
	)
	
	if not exist "%TMP%\repo" (
		mkdir %TMP%\repo 
	)
	mkdir %TMP%\repo\Windows!BIT!
	mkdir %TMP%\repo\Windows!BIT!\Natron-%GITV_NATRON%
	mkdir %TMP%\repo\Windows!BIT!\BuildLogs
	echo R | xcopy /Y /E %TMP%\deploy!BIT! %TMP%\repo\Windows!BIT!\Natron-%GITV_NATRON%
	cd %TMP%\repo\Windows!BIT!
	zip -q -r Natron-%GITV_NATRON%.zip Natron-%GITV_NATRON%
	rmdir /S /Q Natron-%GITV_NATRON%
	echo R | xcopy /Y /E %TMP%\Logs!BIT! %TMP%\repo\Windows!BIT!\BuildLogs
	cd ..
	#rsync -avz -e ssh --progress --delete Windows!BIT! kepzlol@frs.sourceforge.net:/home/frs/project/natron/snapshots
	rsync -avz -e "ssh -i /cygdrive/c/Users/lex/.ssh/id_rsa_build_win" --progress Windows!BIT!/ mrkepzie@vps163799.ovh.net:../www/downloads.natron.fr/Windows/snapshots/!BIT!bit
)

) else (
	timeout 60
)

goto main_loop
cd %CWD%


	
:FAIL
	set errorlevel=1
	exit /b awk
	
:SUCCESS
	
