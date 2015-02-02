
@echo off
set CWD=%cd%
set TMP=%CWD%\autobuild
set GIT_NATRON=https://github.com/MrKepzie/Natron.git
set GIT_IO=https://github.com/MrKepzie/openfx-io.git
set GIT_MISC=https://github.com/devernay/openfx-misc.git
set NATRON_BRANCH=workshop
set OPENFX_BRANCH=master

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
if "%GITV_NATRON%"=="%ORIG_NATRON%" (
	echo "Natron up to date"
) else (
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
if "%GITV_IO%"=="%ORIG_IO%" (
	echo "openfx-io up to date"
) else (
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
if "%GITV_MISC%"=="%ORIG_MISC%" (
	echo "openfx-misc up to date"
) else (
	echo "openfx-misc update needed"
	set BUILD_MISC=1
	set BUILD_ALL=1
	echo %GITV_MISC% > %CWD%\MISC_WORKSHOP
)

cd %TMP%
set i=0
if "%BUILD_ALL%"=="1" (
:FOR_ALL_ARCH
	if "%i%" == "2" (
		goto main_loop
	)
	if "%i%" == "0" (
		set BIT=64
	)
	if "%i%" == "1" (
		set BIT=32
	)
	set i=%i%+1
	goto build_arch
) else (
	timeout 60
	goto main_loop
)


cd %CWD%

:BUILD_ARCH
cd %CWD%
	call build-natron.bat %NATRON_BRANCH% %BIT% 0 Release 1
	cd %CWD%
	call build-plugins.bat %BIT% Release 1
	if exist "%TMP%\repo\Windows%BIT%" (
		rmdir /S /Q %TMP%\repo\Windows%BIT%
	)
	
	if not exist "%TMP%\repo" (
		mkdir %TMP%\repo 
	)
	mkdir %TMP%\repo\Windows%BIT%
	mkdir %TMP%\repo\Windows%BIT%\Natron-%GITV_NATRON%
	xcopy /Y /E %TMP%\deploy %TMP%\repo\Windows%BIT%\Natron-%GITV_NATRON%
	cd %TMP%\repo\Windows%BIT%
	zip -r Natron-%GITV_NATRON%.zip Natron-%GITV_NATRON%
	rmdir /S /Q Natron-%GITV_NATRON%
	cd ..
	rsync -avz -e ssh --delete Windows%BIT% kepzlol@frs.sourceforge.net:/home/frs/project/natron/snapshots
	goto for_all_arch
	
:FAIL
	set errorlevel=1
	exit /b awk
	
:SUCCESS
	