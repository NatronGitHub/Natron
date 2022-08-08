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

file(GLOB HostSupport_HEADERS
    ../libs/OpenFX/HostSupport/include/*.h
    ../libs/OpenFX/include/*.h
    ../libs/OpenFX/include/nuke/*.h
    ../libs/OpenFX/include/tuttle/*.h
    ../libs/OpenFX_extensions/ofxhParametricParam.h
)
set(HostSupport_SOURCES
    ../libs/OpenFX/HostSupport/src/ofxhBinary.cpp
    ../libs/OpenFX/HostSupport/src/ofxhClip.cpp
    ../libs/OpenFX/HostSupport/src/ofxhHost.cpp
    ../libs/OpenFX/HostSupport/src/ofxhImageEffect.cpp
    ../libs/OpenFX/HostSupport/src/ofxhImageEffectAPI.cpp
    ../libs/OpenFX/HostSupport/src/ofxhInteract.cpp
    ../libs/OpenFX/HostSupport/src/ofxhMemory.cpp
    ../libs/OpenFX/HostSupport/src/ofxhParam.cpp
    ../libs/OpenFX/HostSupport/src/ofxhPluginAPICache.cpp
    ../libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp
    ../libs/OpenFX/HostSupport/src/ofxhPropertySuite.cpp
    ../libs/OpenFX/HostSupport/src/ofxhUtilities.cpp
    ../libs/OpenFX_extensions/ofxhParametricParam.cpp
    ${HostSupport_HEADERS}
)
add_library(HostSupport STATIC ${HostSupport_SOURCES})
target_link_libraries(HostSupport
    PRIVATE
        expat
)
target_include_directories(HostSupport
    PUBLIC
        ../libs/OpenFX/include
        ../libs/OpenFX_extensions
        ../libs/OpenFX/HostSupport/include
)
target_compile_definitions(HostSupport
    PUBLIC
        OFX_EXTENSIONS_NUKE
        OFX_EXTENSIONS_TUTTLE
        OFX_EXTENSIONS_VEGAS
        OFX_SUPPORTS_PARAMETRIC
        OFX_EXTENSIONS_TUTTLE
        OFX_EXTENSIONS_NATRON
        OFX_EXTENSIONS_RESOLVE
        OFX_SUPPORTS_OPENGLRENDER
        OFX_SUPPORTS_MULTITHREAD
        OFX_SUPPORTS_DIALOG
)
set_target_properties(HostSupport PROPERTIES POSITION_INDEPENDENT_CODE ON)
