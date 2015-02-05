:: This script assumes you have installed Visual Studio 2010 and  the following libraries:
:: - boost in C:\boost, where libraries are in C:\boost\win32 and C:\boost\x64 downloaded here:
:: http://boost.teeks99.com/

@echo off

set CWD=%cd%

:: Either 32 or 64
set BITS=%1%

if "%3" == "1" (
	set BUILD_DIR=%CWD%\autobuild\build%BITS%
	set DEPLOY_DIR=%CWD%\autobuild\deploy%BITS%
) else (
	set BUILD_DIR=%CWD%\build%BITS%
	set DEPLOY_DIR=%CWD%\build\deploy%BITS%
)

set IO_BRANCH=master
set MISC_BRANCH=master


:: If %4% set you can specify a build condiguration, either Debug or Release 
:: This defaults to Release
if "%2" == "Debug" (
	set CONFIGURATION=Debug
	set CONF_BUILD_SUB_DIR=debug
) else (
	set CONFIGURATION=Release
	set CONF_BUILD_SUB_DIR=release
)

if "%BITS%" == "32" (
	set BUILD_SUB_DIR=win32
	set MSVC_CONF=Win32
	set OFX_CONF_SUB_DIR=Win32
) else if "%BITS%" == "64" (
	set BUILD_SUB_DIR=x64
	set MSVC_CONF=x64
	set OFX_CONF_SUB_DIR=Win64
) else (
	echo Architecture must be either 32 or 64 bits.
	goto fail
)

set PROJECT_OBJ_DIR=%BUILD_SUB_DIR%\%CONF_BUILD_SUB_DIR%


:: Set here the path where the Qt_64bit libraries enclosing directory is located
:: You can download our pre-compiled version here: 
:: https://sourceforge.net/projects/natron/files/Natron_Windows_3rdParty.zip/download 
set DEP_PATH=C:\Users\Lex\Documents\Github\Natron3rdParty

set GIT_IO=https://github.com/MrKepzie/openfx-io.git
set GIT_MISC=https://github.com/devernay/openfx-misc.git

if not exist "%BUILD_DIR%" (
	echo "You should build Natron before building plug-ins"
	goto fail
)

cd %BUILD_DIR%
git clone %GIT_MISC%
cd openfx-misc
git checkout %MISC_BRANCH%
git pull origin %MISC_BRANCH%
git submodule update -i --recursive
git log|head -1|awk "{print $2}" > curVersion.txt
set /p GITV_MISC=<curVersion.txt
echo "Building openfx-misc at commit " %GITV_MISC% %BITS%bit %CONFIGURATION%

cd Misc
devenv Misc.sln /Build "%CONFIGURATION%|%MSVC_CONF%" /Project Support
devenv Misc.sln /Build "%CONFIGURATION%|%MSVC_CONF%" /Project Misc
devenv Misc.sln /Build "%CONFIGURATION%|%MSVC_CONF%" /Project CImg

if exist "%DEPLOY_DIR%\Plugins\Misc.ofx.bundle" (
	rmdir /S /Q %DEPLOY_DIR%\Plugins\Misc.ofx.bundle
)
mkdir %DEPLOY_DIR%\Plugins\Misc.ofx.bundle
xcopy /Y /E %PROJECT_OBJ_DIR%\Misc.ofx.bundle %DEPLOY_DIR%\Plugins\Misc.ofx.bundle
if exist "%DEPLOY_DIR%\Plugins\CImg.ofx.bundle" (
	rmdir /S /Q %DEPLOY_DIR%\Plugins\CImg.ofx.bundle
)
mkdir %DEPLOY_DIR%\Plugins\CImg.ofx.bundle
xcopy /Y /E %PROJECT_OBJ_DIR%\CImg.ofx.bundle %DEPLOY_DIR%\Plugins\CImg.ofx.bundle


cd %BUILD_DIR%
git clone %GIT_IO%
cd openfx-io
git checkout %IO_BRANCH%
git pull origin %IO_BRANCH%
git submodule update -i --recursive
git log|head -1|awk "{print $2}" > curVersion.txt
set /p GITV_IO=<curVersion.txt
echo "Building openfx-io at commit " %GITV_IO% %BITS%bit %CONFIGURATION%

devenv IO.sln /Build "%CONFIGURATION%|%MSVC_CONF%"
if exist "%DEPLOY_DIR%\Plugins\IO.ofx.bundle" (
	rmdir /S /Q %DEPLOY_DIR%\Plugins\IO.ofx.bundle
)
mkdir %DEPLOY_DIR%\Plugins\IO.ofx.bundle
xcopy /Y /E %PROJECT_OBJ_DIR%\IO.ofx.bundle %DEPLOY_DIR%\Plugins\IO.ofx.bundle
copy /Y %DEP_PATH%\ffmpeg_2.4\lib\%BUILD_SUB_DIR%\avcodec-56.dll %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
copy /Y %DEP_PATH%\ffmpeg_2.4\lib\%BUILD_SUB_DIR%\avformat-56.dll %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
copy /Y %DEP_PATH%\ffmpeg_2.4\lib\%BUILD_SUB_DIR%\avutil-54.dll %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
copy /Y %DEP_PATH%\ffmpeg_2.4\lib\%BUILD_SUB_DIR%\swresample-1.dll %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
copy /Y %DEP_PATH%\ffmpeg_2.4\lib\%BUILD_SUB_DIR%\swscale-3.dll %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
copy INRIA.IO.manifest %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
cd %DEPLOY_DIR%\Plugins\IO.ofx.bundle\Contents\%OFX_CONF_SUB_DIR%
mt -manifest INRIA.IO.manifest -outputresource:IO.ofx;2

cd %CWD%

goto success

:FAIL
	set errorlevel=1
	exit /b awk
	
:SUCCESS
	