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
if(WIN32)
    list(APPEND Natron_SOURCES ../Natron.rc)
endif()

set(NATRON_RESOURCES_SOURCE_DIR "${PROJECT_SOURCE_DIR}/Gui/Resources")

############################################
## fontconfig configuration (Resources/etc/fonts/fonts.conf)
## Same as QMAKE_SUBSTITUTES += fonts.conf.in in App.pro, with the
## FC_DEFAULT_FONTS and FC_CACHEDIR values from global.pri.
## See https://github.com/NatronGitHub/Natron/blob/201c9b560d50c33f5761ba16b4a60455f909c455/Gui/Resources/etc/fonts/fonts.conf
set(FC_DEFAULT_FONTS "")
set(FC_CACHEDIR "")
if(APPLE)
    list(APPEND FC_DEFAULT_FONTS
        "<dir>/usr/local/share/fonts</dir>" # Homebrew
        "<dir>/opt/X11/share/fonts</dir>"   # XQuartz
        "<dir>/opt/local/share/fonts</dir>" # MacPorts
        "<dir>/System/Library/Fonts</dir>"
        "<dir>/Network/Library/Fonts</dir>"
        "<dir>/Library/Fonts</dir>"
        "<dir>~/Library/Fonts</dir>"
        "<dir>/System/Library/Assets/com_apple_MobileAsset_Font3</dir>"
        "<dir>/System/Library/Assets/com_apple_MobileAsset_Font4</dir>"
        "<dir>/System/Library/Assets/com_apple_MobileAsset_Font5</dir>"
    )
    # There's no system-default FC_CACHEDIR on macOS
elseif(WIN32)
    list(APPEND FC_DEFAULT_FONTS
        "<dir>WINDOWSFONTDIR</dir>"
        # Windows 10 introduced user fonts, and is the preferred (and default) place to install fonts
        "<dir>~/AppData/Local/Microsoft/Windows/Fonts</dir>"
    )
    list(APPEND FC_CACHEDIR "<cachedir>LOCAL_APPDATA_FONTCONFIG_CACHE</cachedir>")
else()
    list(APPEND FC_DEFAULT_FONTS
        "<dir>/usr/share/fonts</dir>"
        "<dir>/usr/X11/lib/X11/fonts</dir>"
        "<dir>/usr/X11R6/lib/X11/fonts</dir>"
        "<dir>/usr/share/X11/fonts</dir>"
        "<dir>/usr/local/share/fonts</dir>"
    )
    # global.pri has <dir> here, which is a typo: this is a cache directory.
    list(APPEND FC_CACHEDIR "<cachedir>/var/cache/fontconfig</cachedir>")
endif()
string(REPLACE ";" " " FC_DEFAULT_FONTS "${FC_DEFAULT_FONTS}")
string(REPLACE ";" " " FC_CACHEDIR "${FC_CACHEDIR}")
# fonts.conf.in is written for qmake's QMAKE_SUBSTITUTES: quotes are
# escaped (\" and \') and variables are $$NAME.
file(READ "${NATRON_RESOURCES_SOURCE_DIR}/etc/fonts/fonts.conf.in" _fonts_conf)
string(REPLACE "\\\"" "\"" _fonts_conf "${_fonts_conf}")
string(REPLACE "\\'" "'" _fonts_conf "${_fonts_conf}")
string(REPLACE "$$FC_DEFAULT_FONTS" "${FC_DEFAULT_FONTS}" _fonts_conf "${_fonts_conf}")
string(REPLACE "$$FC_CACHEDIR" "${FC_CACHEDIR}" _fonts_conf "${_fonts_conf}")
set(NATRON_FONTS_CONF "${CMAKE_CURRENT_BINARY_DIR}/Resources/etc/fonts/fonts.conf")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/fonts.conf.tmp" "${_fonts_conf}")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/fonts.conf.tmp" "${NATRON_FONTS_CONF}" COPYONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${NATRON_RESOURCES_SOURCE_DIR}/etc/fonts/fonts.conf.in")
file(GLOB NATRON_FONTS_CONF_D "${NATRON_RESOURCES_SOURCE_DIR}/etc/fonts/conf.d/*")

# OpenColorIO configs that exist in NATRON_OCIO_CONFIGS_DIR
set(NATRON_OCIO_CONFIG_DIRS "")
if(NATRON_OCIO_CONFIGS_DIR)
    foreach(_ocio IN LISTS NATRON_OCIO_CONFIGS)
        if(IS_DIRECTORY "${NATRON_OCIO_CONFIGS_DIR}/${_ocio}")
            list(APPEND NATRON_OCIO_CONFIG_DIRS "${NATRON_OCIO_CONFIGS_DIR}/${_ocio}")
        else()
            message(WARNING "OpenColorIO config \"${_ocio}\" not found in NATRON_OCIO_CONFIGS_DIR=${NATRON_OCIO_CONFIGS_DIR}")
        endif()
    endforeach()
endif()

if(APPLE)
    # Natron.app, as built by App/App.pro (QMAKE_INFO_PLIST, ICON,
    # QMAKE_PKGINFO_TYPEINFO, QMAKE_BUNDLE_DATA).
    set(NATRON_BUNDLE_ICON "${NATRON_RESOURCES_SOURCE_DIR}/Images/natronIcon.icns")
    set(NATRON_BUNDLE_PROJECT_ICON "${NATRON_RESOURCES_SOURCE_DIR}/Images/natronProjectIcon_osx.icns")
    set(NATRON_BUNDLE_SIGNATURE "Ntrn") # QMAKE_PKGINFO_TYPEINFO
    set(NATRON_PKGINFO "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo")
    file(WRITE "${NATRON_PKGINFO}.tmp" "APPL${NATRON_BUNDLE_SIGNATURE}\n")
    configure_file("${NATRON_PKGINFO}.tmp" "${NATRON_PKGINFO}" COPYONLY)

    # NatronInfo.plist.in is written for qmake (QMAKE_SUBSTITUTES, then
    # QMAKE_INFO_PLIST): turn it into a template for CMake's
    # MACOSX_BUNDLE_INFO_PLIST, which is filled from the MACOSX_BUNDLE_*
    # target properties below. Unlike QMAKE_SUBSTITUTES, this keeps the
    # double quotes of the XML declaration and DOCTYPE (issue #1074);
    # quotes escaped for qmake (\") are unescaped.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/NatronInfo.plist.in" _plist)
    string(REPLACE "\\\"" "\"" _plist "${_plist}")
    string(REPLACE "\\'" "'" _plist "${_plist}")
    string(REPLACE "$$VERSION" "\${MACOSX_BUNDLE_BUNDLE_VERSION}" _plist "${_plist}")
    string(REPLACE "fr.inria.@EXECUTABLE@" "\${MACOSX_BUNDLE_GUI_IDENTIFIER}" _plist "${_plist}")
    string(REPLACE "@EXECUTABLE@" "\${MACOSX_BUNDLE_EXECUTABLE_NAME}" _plist "${_plist}")
    string(REPLACE "@ICON@" "\${MACOSX_BUNDLE_ICON_FILE}" _plist "${_plist}")
    string(REPLACE "@TYPEINFO@" "${NATRON_BUNDLE_SIGNATURE}" _plist "${_plist}")
    if(_plist MATCHES "\\$\\$|@[A-Z_]+@")
        message(FATAL_ERROR "App/NatronInfo.plist.in contains a qmake substitution that App/CMakeLists.txt does not handle")
    endif()
    set(NATRON_INFO_PLIST_TEMPLATE "${CMAKE_CURRENT_BINARY_DIR}/NatronInfo.plist.cmake.in")
    file(WRITE "${NATRON_INFO_PLIST_TEMPLATE}.tmp" "${_plist}")
    configure_file("${NATRON_INFO_PLIST_TEMPLATE}.tmp" "${NATRON_INFO_PLIST_TEMPLATE}" COPYONLY)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/NatronInfo.plist.in")

    set(NATRON_BUNDLE_FILES
        "${NATRON_BUNDLE_ICON}"
        "${NATRON_BUNDLE_PROJECT_ICON}"
        "${NATRON_PKGINFO}"
        "${NATRON_FONTS_CONF}"
        ${NATRON_FONTS_CONF_D}
    )
    list(APPEND Natron_SOURCES ${NATRON_BUNDLE_FILES})
    set_source_files_properties("${NATRON_BUNDLE_ICON}" "${NATRON_BUNDLE_PROJECT_ICON}"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${NATRON_PKGINFO}"
        PROPERTIES MACOSX_PACKAGE_LOCATION .)
    set_source_files_properties("${NATRON_FONTS_CONF}"
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources/etc/fonts)
    set_source_files_properties(${NATRON_FONTS_CONF_D}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources/etc/fonts/conf.d)
    source_group("Bundle Resources" FILES ${NATRON_BUNDLE_FILES})
endif()

add_executable(Natron ${Natron_SOURCES})
if(APPLE)
    get_filename_component(_icon_name "${NATRON_BUNDLE_ICON}" NAME)
    set_target_properties(Natron PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${NATRON_INFO_PLIST_TEMPLATE}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} Copyright (c) 2023 The Natron Developers"
        MACOSX_BUNDLE_ICON_FILE "${_icon_name}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "fr.inria.${PROJECT_NAME}" # QMAKE_TARGET_BUNDLE_PREFIX = fr.inria
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2023 The Natron Developers"
    )
    # OpenColorIO-Configs.pri: QMAKE_BUNDLE_DATA += OCIO
    # (Contents/Resources/OpenColorIO-Configs). Copied after each link of
    # Natron; "cmake --install" copies them again.
    foreach(_ocio_dir IN LISTS NATRON_OCIO_CONFIG_DIRS)
        get_filename_component(_ocio_name "${_ocio_dir}" NAME)
        add_custom_command(TARGET Natron POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${_ocio_dir}"
                "$<TARGET_BUNDLE_CONTENT_DIR:Natron>/Resources/OpenColorIO-Configs/${_ocio_name}"
            VERBATIM)
    endforeach()
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

############################################
## Install rules. See NATRON_INSTALL_LAYOUT in the top-level CMakeLists.txt
## and Documentation/source/maintainers/building.rst.
install(TARGETS Natron
    BUNDLE DESTINATION "${NATRON_INSTALL_BUNDLEDIR}"
    RUNTIME DESTINATION "${NATRON_INSTALL_BINDIR}")

if(NATRON_OCIO_CONFIG_DIRS)
    install(DIRECTORY ${NATRON_OCIO_CONFIG_DIRS}
        DESTINATION "${NATRON_INSTALL_OCIODIR}")
endif()

if(NATRON_INSTALL_LAYOUT STREQUAL "FHS")
    install(FILES ../Gui/Resources/Applications/fr.natron.Natron.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES ../Gui/Resources/Metainfo/fr.natron.Natron.appdata.xml
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
    install(FILES ../Gui/Resources/Mime/x-natron.xml
        DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")
    install(FILES ../Gui/Resources/Images/natronIcon256_linux.png
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps")
    install(FILES ../Gui/Resources/Images/natronProjectIcon_linux.png
        DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps")
else()
    # PORTABLE: the layout of the binary installers (tools/jenkins/build-*-installer.sh).
    # On macOS, the icons, fonts.conf and conf.d are already in Natron.app.
    if(NOT APPLE)
        install(FILES "${NATRON_FONTS_CONF}"
            DESTINATION "${NATRON_INSTALL_RESOURCESDIR}/etc/fonts")
        install(FILES ${NATRON_FONTS_CONF_D}
            DESTINATION "${NATRON_INSTALL_RESOURCESDIR}/etc/fonts/conf.d")
        if(WIN32)
            install(FILES ../Gui/Resources/Images/natronProjectIcon_windows.ico
                DESTINATION "${NATRON_INSTALL_RESOURCESDIR}/pixmaps")
        else()
            install(FILES ../Gui/Resources/Images/natronIcon256_linux.png
                ../Gui/Resources/Images/natronProjectIcon_linux.png
                DESTINATION "${NATRON_INSTALL_RESOURCESDIR}/pixmaps")
        endif()
    endif()
    install(FILES ../Gui/Resources/Stylesheets/mainstyle.qss
        DESTINATION "${NATRON_INSTALL_RESOURCESDIR}/stylesheets")
    install(DIRECTORY ../Gui/Resources/PyPlugs/
        DESTINATION "${NATRON_INSTALL_PLUGINSDIR}/PyPlugs")
    if(NATRON_INSTALL_DOCDIR)
        install(FILES ../LICENSE.txt DESTINATION "${NATRON_INSTALL_DOCDIR}")
    endif()
endif()
