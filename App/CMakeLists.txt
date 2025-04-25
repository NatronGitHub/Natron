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

set(Natron_SOURCES NatronApp_main.cpp)
add_executable(Natron ${Natron_SOURCES})
if(WIN32)
    target_sources(Natron PRIVATE ../Natron.rc)
endif()
if(APPLE)
    set_target_properties(Natron PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} Copyright (c) 2023 The Natron Developers"
        MACOSX_BUNDLE_ICON_FILE Icon.icns
        MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_NAME}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2023 The Natron Developers"
    )
endif()
target_link_libraries(Natron
    PRIVATE
        NatronGui
        Python3::Python
)
target_include_directories(Natron
    PRIVATE
        ..
        ../Gui
        ../Engine
        ../Global
)

install(TARGETS Natron DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES ../Gui/Resources/Applications/fr.natron.Natron.desktop
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES ../Gui/Resources/Metainfo/fr.natron.Natron.appdata.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
install(FILES ../Gui/Resources/Mime/x-natron.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime")
install(FILES ../Gui/Resources/Images/natronIcon256_linux.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps")
install(FILES ../Gui/Resources/Images/natronProjectIcon_linux.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps")

IF(WIN32 AND DEPLOYQT_FOUND)
    SET(CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/install_deps_${CMAKE_BUILD_TYPE}.cmake)

    # Find and install needed DLLs
    file(GENERATE OUTPUT
    "${CONFIG_FILE}" CONTENT 
    [[
        EXECUTE_PROCESS(COMMAND del ${INSTALL_BINPATH}/*.dll)
        SET(TARGET_APP $<TARGET_FILE:Natron>)
            FILE(GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR deps_resolved
            UNRESOLVED_DEPENDENCIES_VAR deps_unresolved
            EXECUTABLES ${TARGET_APP}
            DIRECTORIES ${CMAKELIBPATH}
            PRE_EXCLUDE_REGEXES "api-ms-*" "ext-ms-*"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" 
        )
        MESSAGE(STATUS "Resolving runtime dependencies for ${TARGET_APP}")
        FOREACH(dep ${deps_resolved})
            FILE(INSTALL ${dep} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
            MESSAGE(STATUS "Installing ${dep}")
        ENDFOREACH()
        FOREACH(dep ${deps_unresolved})
            MESSAGE(WARNING "Runtime dependency ${dep} could not be resolved.")
        ENDFOREACH()
        EXECUTE_PROCESS(COMMAND ${DEPLOYQT_EXE} --no-angle --compiler-runtime ${INSTALL_BINPATH}/Natron.exe)
    ]])
        
    INSTALL(CODE "SET(INSTALL_BINPATH \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")")
    INSTALL(CODE "SET(DEPLOYQT_EXE \"${DEPLOYQT_EXE}\")")
    INSTALL(CODE "SET(CMAKELIBPATH \"${CMAKE_SYSTEM_LIBRARY_PATH};${CMAKE_MINGW_SYSTEM_LIBRARY_PATH};${MINGWPATH}\")")
    INSTALL(SCRIPT ${CONFIG_FILE})
    INSTALL(DIRECTORY ${Python3_LIBRARY_DIRS}/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}
            DESTINATION lib
            PATTERN "*.pyc" EXCLUDE)
ENDIF()