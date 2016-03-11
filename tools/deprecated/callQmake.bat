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
setlocal enabledelayedexpansion
if "%1"=="64" (
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64\vcvars64.bat"
) else (
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"
)

qmake -r -tp vc -spec C:\Qt\4.8.6_win32\mkspecs\win32-msvc2010 CONFIG+=%1bit
