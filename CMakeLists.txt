# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2022 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

cmake_minimum_required(VERSION 3.16.7)

project(Natron
    VERSION "2.5.0"
    DESCRIPTION "Open Source Compositing Software"
    HOMEPAGE_URL "https://natrongithub.github.io/"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 11)

option(NATRON_BUILD_TESTS "build the Natron test suite" ON)

find_package(Boost COMPONENTS headers serialization)
find_package(Python3 COMPONENTS Interpreter Development)
find_package(Qt5 5.15 CONFIG REQUIRED COMPONENTS Core Gui Network Widgets Concurrent)
find_package(Shiboken2 5.15 CONFIG REQUIRED COMPONENTS libshiboken2 shiboken2)
find_package(PySide2 5.15 CONFIG REQUIRED COMPONENTS pyside2)
set(QT_VERSION_MAJOR 5)
get_target_property(PYSIDE_INCLUDE_DIRS PySide2::pyside2 INTERFACE_INCLUDE_DIRECTORIES)
string(REGEX REPLACE "(.*)QtCore$" "\\1" Qt5_INCLUDE_DIR ${Qt5Core_INCLUDE_DIRS})
set(QTCORE_INCLUDE_DIRS ${Qt5_INCLUDE_DIR} ${Qt5Core_INCLUDE_DIRS})
set(QTGUI_INCLUDE_DIRS ${Qt5Gui_INCLUDE_DIRS})
set(QTWIDGETS_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})

find_package(expat REQUIRED)

if(UNIX AND NOT APPLE)
    find_package(ECM NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
    find_package(X11 REQUIRED)
    find_package(Wayland COMPONENTS Client Egl)
elseif(APPLE)
    enable_language(OBJCXX)
elseif(WINDOWS)
    enable_language(RC)
    if (NOT MSVC)
        set(CMAKE_RC_COMPILER_INIT windres)
        set(CMAKE_RC_COMPILE_OBJECT
            "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    endif()
endif()

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

enable_testing()

add_subdirectory(libs)

add_subdirectory(HostSupport)

add_subdirectory(Engine)

add_subdirectory(PythonBin)

add_subdirectory(Renderer)

add_subdirectory(Gui)

if(NATRON_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

add_subdirectory(App)
