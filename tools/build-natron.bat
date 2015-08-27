:: ***** BEGIN LICENSE BLOCK *****
:: This file is part of Natron <http://www.natron.fr/>,
:: Copyright (C) 2015 INRIA and Alexandre Gauthier
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

:: This script assumes you have installed Visual Studio 2010 and  the following libraries:
:: - Qt 32 bit in C:\Qt\Qt4.8.6_win32 downloaded here:
:: http://download.qt.io/archive/qt/4.8/4.8.6/ (for Visual 2010)
:: - boost in C:\boost, where libraries are in C:\boost\win32 and C:\boost\x64 downloaded here:
:: http://boost.teeks99.com/
:: - glew in C:\glew, where libraries are in C:\glew\bin\win32 and C:\glew\bin\x64, downloaded here:
:: https://sourceforge.net/projects/glew/files/glew/1.12.0/glew-1.12.0-win32.zip/download
:@echo off

goto start

:SetFromReg
FOR /F "tokens=2,*" %%A IN ('REG query "%~1" /v "%~2"^|find /I "REG_"') DO (
    call set %~3=%%B
)
goto :EOF

:start

set CWD=%cd%


:: Set here the path where the Qt_64bit libraries enclosing directory is located
:: You can download our pre-compiled version here: 
:: https://sourceforge.net/projects/natron/files/Natron_Windows_3rdParty.zip/download 
set DEP_PATH=C:\Users\Lex\Documents\Github\Natron3rdParty

:: URL to Github
set GIT_NATRON=https://github.com/MrKepzie/Natron.git

:: The branch to fetch from Natron
set NATRON_BRANCH=%1%

if "%NATRON_BRANCH%" == "workshop" (
	set WITH_PYTHON=1
) else (
	set WITH_PYTHON=0
)

:: Either 32 or 64
set BITS=%2%

:: If set to 1 this will keep the previous BUILD_DIR directory
set KEEP_PREVIOUS=%3%

:: If %4% set you can specify a build condiguration, either Debug or Release 
:: This defaults to Release
if "%4" == "Debug" (
	set CONFIGURATION=Debug
	set CONF_BUILD_SUB_DIR=debug
) else (
	set CONFIGURATION=Release
	set CONF_BUILD_SUB_DIR=release
)

if "%5" == "1" (
	set BUILD_DIR=%CWD%\autobuild\build%BITS%
	set DEPLOY_DIR=%CWD%\autobuild\deploy%BITS%
	set LOGS_DIR=%CWD%\autobuild\Logs%BITS%
) else (
	set BUILD_DIR=%CWD%\build%BITS%
	set DEPLOY_DIR=%CWD%\build%BITS%\deploy%BITS%
	set LOGS_DIR=%CWD%\build%BITS%\Logs%BITS%
)



::set QMAKE_BINARY=C:\Qt\4.8.6_win32\bin\qmake.exe

if "%BITS%" == "32" (
	echo Building Natron x86 32 bit %CONFIGURATION%.
	set BUILD_SUB_DIR=win32
	set QT_LIBRARIES_DIR=C:\Qt\4.8.6_win32
	set PYTHON_DLL_DIR=C:\Windows\SysWOW64
	set PYTHON_DIR=C:\Python34_win32
	set MSVC_CONF=Win32
) else if "%BITS%" == "64" (
	echo Building Natron x86 64 bit %CONFIGURATION%.
	set BUILD_SUB_DIR=x64
	set QT_LIBRARIES_DIR=%DEP_PATH%\Qt4.8.6_64bit
	set PYTHON_DLL_DIR=C:\Windows\System32
	set PYTHON_DIR=C:\Python34
	set MSVC_CONF=x64
) else (
	echo Architecture must be either 32 or 64 bits.
	goto fail
)

set PROJECT_OBJ_DIR=%BUILD_SUB_DIR%\%CONF_BUILD_SUB_DIR%

if not "%KEEP_PREVIOUS%" == "1" (
	if exist "%BUILD_DIR%" (
		rmdir /S /Q %BUILD_DIR%
	)
)
if not exist "%BUILD_DIR%" (
	mkdir %BUILD_DIR%
)


cd %BUILD_DIR%
git clone %GIT_NATRON%
cd Natron
git checkout %NATRON_BRANCH%
git pull origin %NATRON_BRANCH%
git submodule update -i --recursive
git log|head -1|awk "{print $2}" > curVersion.txt
set /p GITV_NATRON=<curVersion.txt
echo "Building Natron at commit " %GITV_NATRON%

setlocal EnableDelayedExpansion

if exist "config.pri" (
	del config.pri 
)

copy /Y %CWD%\..\config.pri config.pri
copy /Y %CWD%\callQmake.bat callQmake.bat
::start /i /b /wait callQmake.bat %BITS%


set Platform=
REM Special handling for Path
call :SetFromReg "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" Path Path
setlocal
set u=
call :SetFromReg "HKCU\Environment" Path u
endlocal&if not "%Path%"=="" if not "%u%"=="" set Path=%Path%;%u%

::start /i /wait explorer.exe cmd /k call callQmake.bat %BITS%
call callQmake.bat %BITS%
call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\vcvars64.bat"
:: By default Visual Studio 2010 solution files build for 32 bit. Moreover it does not
:: provide any way to change to /MACHINE:X64 from command-line, hence we do it by hend using sed
:: See http://www.cmake.org/Bug/view.php?id=11240 for reference
setlocal EnableDelayedExpansion

if "%BITS%" == "64" (
	::Force x64 because by default visual 2010 produces 32bit binaries
	sed -e "/\/ResourceCompile>/a <Lib>\n    <TargetMachine>MachineX64</TargetMachine>\n</Lib>" -i Engine\Engine.vcxproj Gui\Gui.vcxproj HostSupport\HostSupport.vcxproj

	sed -e "/<SubSystem>/ s/Windows/Console/g" -i App\Natron.vcxproj
	
	:: Qmake in the path is the Qmake 32 bit hence the visual studio solution is setup to build against
	:: 32bit libraries of Qt. The following is to use our compiled version of Qt 64bit 
	::for /f "tokens=*" %a in ('echo %DEP_PATH%^| sed "s/\\/\\\\/g"') do set DEP_PATH_ESCAPED=%a
	sed -e "/<AdditionalDependencies>/ s/c:\\Qt\\4.8.6_win32/c:\\Users\\Lex\\Documents\\Github\\Natron3rdParty\\Qt4.8.6_64bit/g" -i App\Natron.vcxproj Renderer\NatronRenderer.vcxproj Tests\Tests.vcxproj
)
set errorlevel=0
devenv Project.sln /Build "%CONFIGURATION%|%MSVC_CONF%" /Project Natron
if "%errorlevel%" == "1" (
	goto fail
)
set errorlevel=0
devenv Project.sln /Build "%CONFIGURATION%|%MSVC_CONF%" /Project NatronRenderer
if "%errorlevel%" == "1" (
	goto fail
)
:: Deploy Natron in Release mode
if "%CONFIGURATION%" == "Release" (
	if exist "%DEPLOY_DIR%" (
		rmdir /S /Q %DEPLOY_DIR%
	)
	mkdir %DEPLOY_DIR%
	mkdir %LOGS_DIR%
	mkdir %DEPLOY_DIR%\bin
	mkdir %DEPLOY_DIR%\Plugins
	mkdir %DEPLOY_DIR%\Resources
	mkdir %DEPLOY_DIR%\Resources\OpenColorIO-Configs
	
	::Copy build logs
	copy /Y Engine\%PROJECT_OBJ_DIR%\.obj\Engine.log %LOGS_DIR%
	copy /Y HostSupport\%PROJECT_OBJ_DIR%\.obj\HostSupport.log %LOGS_DIR%
	copy /Y Gui\%PROJECT_OBJ_DIR%\.obj\Gui.log %LOGS_DIR%
	copy /Y App\%PROJECT_OBJ_DIR%\.obj\Natron.log %LOGS_DIR%
	copy /Y Renderer\%PROJECT_OBJ_DIR%\.obj\NatronRenderer.log %LOGS_DIR%
	
	echo R | xcopy  /Y /E Gui\Resources\OpenColorIO-Configs %DEPLOY_DIR%\Resources\OpenColorIO-Configs
	del  %DEPLOY_DIR%\Resources\OpenColorIO-Configs\.git
	copy /Y App\%PROJECT_OBJ_DIR%\Natron.exe %DEPLOY_DIR%\bin
	copy /Y Renderer\%PROJECT_OBJ_DIR%\NatronRenderer.exe %DEPLOY_DIR%\bin
	copy /Y LICENSE.txt %DEPLOY_DIR%
	copy /Y Gui\Resources\Images\natronIcon256_windows.ico %DEPLOY_DIR%\bin
	copy /Y Gui\Resources\Images\natronProjectIcon_windows.ico %DEPLOY_DIR%\bin
	
	mkdir %DEPLOY_DIR%\bin\accessible
	xcopy  /Y /E %QT_LIBRARIES_DIR%\plugins\accessible %DEPLOY_DIR%\bin\accessible
	mkdir %DEPLOY_DIR%\bin\bearer
	xcopy  /Y /E %QT_LIBRARIES_DIR%\plugins\bearer %DEPLOY_DIR%\bin\bearer
	mkdir %DEPLOY_DIR%\bin\codecs
	xcopy  /Y /E %QT_LIBRARIES_DIR%\plugins\codecs %DEPLOY_DIR%\bin\codecs
	mkdir %DEPLOY_DIR%\bin\iconengines
    xcopy  /Y /E %QT_LIBRARIES_DIR%\plugins\iconengines %DEPLOY_DIR%\bin\iconengines
	mkdir %DEPLOY_DIR%\bin\imageformats
	xcopy  /Y /E %QT_LIBRARIES_DIR%\plugins\imageformats %DEPLOY_DIR%\bin\imageformats
	
	copy /Y %QT_LIBRARIES_DIR%\bin\QtCore4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtGui4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtNetwork4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtOpenGL4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtWebKit4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtXml4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtXmlPatterns4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtSvg4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtSql4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtScriptTools4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtScript4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtMultimedia4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtHelp4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtCLucene4.dll %DEPLOY_DIR%\bin
	copy /Y %QT_LIBRARIES_DIR%\bin\QtDeclarative4.dll %DEPLOY_DIR%\bin
	
	copy /Y %DEP_PATH%\cairo_1.12\lib\%BUILD_SUB_DIR%\cairo.dll %DEPLOY_DIR%\bin
	copy /Y C:\glew\bin\Release\%BUILD_SUB_DIR%\glew32.dll %DEPLOY_DIR%\bin
	copy /Y C:\boost\%BUILD_SUB_DIR%\boost_serialization-vc100-mt-1_57.dll %DEPLOY_DIR%\bin
	
	if "%WITH_PYTHON%" == "1" (
		copy /Y %PYTHON_DIR%\Lib\site-packages\PySide\shiboken-python3.4.dll %DEPLOY_DIR%\bin
		copy /Y %PYTHON_DIR%\Lib\site-packages\PySide\pyside-python3.4.dll %DEPLOY_DIR%\bin
		copy /Y %PYTHON_DLL_DIR%\python34.dll %DEPLOY_DIR%\bin	
		mkdir %DEPLOY_DIR%\bin\DLLs
		xcopy /Y /E /Q %PYTHON_DIR%\DLLs %DEPLOY_DIR%\bin\DLLs
		mkdir %DEPLOY_DIR%\bin\Lib
		xcopy /Y /E /Q %PYTHON_DIR%\Lib %DEPLOY_DIR%\bin\Lib
		for /d %%G in (%DEPLOY_DIR%\bin\Lib\__pycache__,%DEPLOY_DIR%\bin\Lib\*\__pycache__,%DEPLOY_DIR%\bin\Lib\site-packages) do rd /s /q "%%~G"
		mkdir %DEPLOY_DIR%\bin\Lib\site-packages
		mkdir %DEPLOY_DIR%\Plugins\PySide
		copy /Y %PYTHON_DIR%\Lib\site-packages\PySide\__init__.py %DEPLOY_DIR%\Plugins\PySide
		copy /Y %PYTHON_DIR%\Lib\site-packages\PySide\_utils.py %DEPLOY_DIR%\Plugins\PySide
		copy /Y %PYTHON_DIR%\Lib\site-packages\PySide\*.pyd %DEPLOY_DIR%\Plugins\PySide
	)
	
	if "%BITS%" == "32" (
		copy /Y "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\redist\x86\Microsoft.VC100.CRT\msvcp100.dll" %DEPLOY_DIR%\bin
		copy /Y "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\redist\x86\Microsoft.VC100.CRT\msvcr100.dll" %DEPLOY_DIR%\bin
	) else (
		copy /Y "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\redist\x64\Microsoft.VC100.CRT\msvcp100.dll" %DEPLOY_DIR%\bin
		copy /Y "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\redist\x64\Microsoft.VC100.CRT\msvcr100.dll" %DEPLOY_DIR%\bin
	)
	
	cd %DEPLOY_DIR%\Resources
	touch qt.conf
	echo [Paths] >> qt.conf
	echo Plugins = QtPlugins >> qt.conf

) 
set errorlevel=0
cd %CWD%

goto success

:FAIL
	set errorlevel=1
	exit /b awk
	
:SUCCESS
	
