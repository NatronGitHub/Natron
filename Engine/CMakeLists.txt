# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
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

include(FindPkgConfig)

pkg_check_modules(Cairo REQUIRED IMPORTED_TARGET cairo fontconfig)

file(GLOB NatronEngine_HEADERS *.h)
file(GLOB NatronEngine_SOURCES *.cpp)

set(PYENGINE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Qt${QT_VERSION_MAJOR})
set(PYENGINE_INCLUDE_DIRS ${PYSIDE_INCLUDE_DIRS} ${PYSIDE_INCLUDE_DIRS}/QtCore)
list(APPEND PYENGINE_INCLUDE_DIRS ${QTCORE_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
list(TRANSFORM PYENGINE_INCLUDE_DIRS PREPEND "-I" OUTPUT_VARIABLE PYENGINE_INCS)
set(PYENGINE_HEADER PySide2_Engine_Python.h)
set(POST_SHIBOKEN ../tools/utils/runPostShiboken2.sh)

set(shiboken_args
    "--enable-parent-ctor-heuristic" "--use-isnull-as-nb_nonzero"
    "--avoid-protected-hack" "--enable-pyside-extensions"
    "-I." "-I.." "-I../Global" "-I../libs/OpenFX/include" ${PYENGINE_INCS}
    "-T${PYSIDE_TYPESYSTEMS}" "--output-directory=${PYENGINE_OUT}"
    "${PYENGINE_HEADER}" "typesystem_engine.xml")
if(APPLE)
    list(PREPEND shiboken_args -I${Qt5_DIR}/../../../include)
 endif()
 
execute_process(COMMAND ${Python3_EXECUTABLE} ../tools/utils/sourceList.py typesystem_engine.xml ${PYENGINE_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE PyEngine_SOURCES)
set_property(SOURCE ${PyEngine_SOURCES} PROPERTY SKIP_AUTOGEN ON)
add_custom_command(OUTPUT ${PyEngine_SOURCES}
    COMMAND Shiboken2::shiboken2 ${shiboken_args}
    COMMAND bash ${POST_SHIBOKEN} ${PYENGINE_OUT}/NatronEngine natronengine
    DEPENDS ${PYENGINE_HEADER} ${NatronEngine_HEADERS} ${POST_SHIBOKEN} typesystem_engine.xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX AND NOT APPLE)
    set(XDG_LIBS ${X11_LIBRARIES})
    if(Wayland_FOUND)
        set(XDG_LIBS Wayland::Client Wayland::Egl ${XDG_LIBS})
        set(XDG_DEFS __NATRON_WAYLAND__)
    endif()
endif()

if(APPLE)
    list(APPEND NatronEngine_SOURCES QUrlFix.mm)
endif()

list(APPEND NatronEngine_SOURCES
    ${NatronEngine_SOURCES}
    ${PyEngine_SOURCES}
    ../Global/glad_source.c
    ../Global/FStreamsSupport.cpp
    ../Global/ProcInfo.cpp
    ../Global/PythonUtils.cpp
    ../Global/StrUtils.cpp
    ../libs/SequenceParsing/SequenceParsing.cpp
)
add_library(NatronEngine STATIC ${NatronEngine_HEADERS} ${NatronEngine_SOURCES})
target_link_libraries(NatronEngine
    PUBLIC
        HostSupport
        Boost::headers
        Boost::serialization
        PkgConfig::Cairo
    PRIVATE
        Shiboken2::libshiboken
        PySide2::pyside2
        Qt5::Core
        Qt5::Concurrent
        Qt5::Network
        Python3::Module
        ${XDG_LIBS}
        ceres
        hoedown
        mv
        openMVG
        tess
)

if(WIN32)
    #System library is required on windows to map network share names from drive letters
    target_link_libraries(NatronEngine PRIVATE mpr)
endif()

target_include_directories(NatronEngine
    PUBLIC
        ../Global/glad$<IF:$<CONFIG:Debug>,Deb,Rel>/include
    PRIVATE
        ${PYSIDE_INCLUDE_DIRS}/QtCore
        .
        ..
        ../Global
        ../libs/SequenceParsing
        ${X11_INCLUDE_DIR}
)
target_compile_definitions(NatronEngine
    PRIVATE
        NATRON_CUSTOM_BUILD_USER_TOKEN=${BUILD_USER_NAME}
        NATRON_BUILD_NUMBER=0
        ${XDG_DEFS}
)
