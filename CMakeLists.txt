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

cmake_minimum_required(VERSION 3.16.7)

project(Natron
    VERSION "2.6.0"
    DESCRIPTION "Open Source Compositing Software"
    HOMEPAGE_URL "https://natrongithub.github.io/"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)

include(GNUInstallDirs)

option(NATRON_QT6 "use Qt6" OFF)
option(NATRON_SYSTEM_LIBS "use system versions of dependencies instead of bundled ones" OFF)
option(NATRON_BUILD_TESTS "build the Natron test suite" ON)

# Install layout ("cmake --install"), see Documentation/source/maintainers/building.rst.
# FHS is the default on every platform, and installs the same files to the same places as before
# NATRON_INSTALL_LAYOUT existed. The installer scripts (tools/jenkins) pass -DNATRON_INSTALL_LAYOUT=PORTABLE.
set(NATRON_INSTALL_LAYOUT "FHS" CACHE STRING
    "install layout: FHS (bin/, share/, for Linux distributions and developers) or PORTABLE (the layout of the binary installers: bin/, Resources/, Plugins/ on Linux and Windows; everything inside Natron.app on macOS)")
set_property(CACHE NATRON_INSTALL_LAYOUT PROPERTY STRINGS FHS PORTABLE)
if(NOT NATRON_INSTALL_LAYOUT MATCHES "^(FHS|PORTABLE)$")
    message(FATAL_ERROR "NATRON_INSTALL_LAYOUT must be FHS or PORTABLE, not \"${NATRON_INSTALL_LAYOUT}\"")
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/OpenColorIO-Configs")
    set(NATRON_DEFAULT_OCIO_CONFIGS_DIR "${PROJECT_SOURCE_DIR}/OpenColorIO-Configs")
else()
    set(NATRON_DEFAULT_OCIO_CONFIGS_DIR "")
endif()
set(NATRON_OCIO_CONFIGS_DIR "${NATRON_DEFAULT_OCIO_CONFIGS_DIR}" CACHE PATH
    "directory containing the OpenColorIO-Configs to bundle and install (https://github.com/NatronGitHub/OpenColorIO-Configs); empty: none")
set(NATRON_OCIO_CONFIGS "blender;natron;nuke-default" CACHE STRING
    "OpenColorIO configs from NATRON_OCIO_CONFIGS_DIR to bundle and install")

# Install directories, relative to CMAKE_INSTALL_PREFIX. Defaults depend on the layout;
# each one can be overridden with -D<name>=<dir>. They are STRING, not PATH: CMake would turn
# a relative PATH given on the command line into an absolute path.
# Natron looks for its resources relative to its executable:
#   OpenColorIO configs: ../Resources/OpenColorIO-Configs (all platforms),
#                        ../share/OpenColorIO-Configs and /usr/share/OpenColorIO-Configs (Linux)
#                        (Engine/Settings.cpp)
#   fontconfig:          ../Resources/etc/fonts (Engine/AppManager.cpp)
#   PyPlugs:             ../Plugins/PyPlugs, and /usr/share/Natron/Plugins on Linux (Engine/AppManager.cpp)
if(NATRON_INSTALL_LAYOUT STREQUAL "PORTABLE" AND APPLE)
    set(_bundledir ".")
    set(_bindir "Natron.app/Contents/MacOS")
    set(_resourcesdir "Natron.app/Contents/Resources")
    set(_pyplugsdir "Natron.app/Contents/Plugins/PyPlugs")
    set(_docdir "")
    # tools/jenkins/build-OSX-installer.sh runs Tests from Natron.app/Contents/MacOS (and removes
    # it before signing), and its relative rpaths only resolve from there.
    set(_testsdir "Natron.app/Contents/MacOS")
elseif(NATRON_INSTALL_LAYOUT STREQUAL "PORTABLE")
    set(_bundledir ".")
    set(_bindir "bin")
    set(_resourcesdir "Resources")
    set(_pyplugsdir "Plugins/PyPlugs")
    set(_docdir "docs/natron")
    set(_testsdir "bin")
else()
    set(_bundledir "${CMAKE_INSTALL_BINDIR}")
    set(_bindir "${CMAKE_INSTALL_BINDIR}")
    if(WIN32)
        # On Windows, Natron only looks in ../Resources (see above).
        set(_resourcesdir "Resources")
    else()
        set(_resourcesdir "")
    endif()
    if(APPLE OR WIN32)
        set(_pyplugsdir "")
    else()
        set(_pyplugsdir "${CMAKE_INSTALL_DATADIR}/Natron/Plugins")
    endif()
    set(_docdir "")
    set(_testsdir "${CMAKE_INSTALL_BINDIR}")
endif()
if(NATRON_INSTALL_LAYOUT STREQUAL "FHS" AND NOT WIN32 AND NOT APPLE)
    set(_ociodir "${CMAKE_INSTALL_DATADIR}/OpenColorIO-Configs")
elseif(_resourcesdir)
    set(_ociodir "${_resourcesdir}/OpenColorIO-Configs")
else()
    set(_ociodir "") # macOS FHS: the configs are inside Natron.app
endif()
set(NATRON_INSTALL_BUNDLEDIR "${_bundledir}" CACHE STRING "install directory of Natron.app (macOS)")
set(NATRON_INSTALL_BINDIR "${_bindir}" CACHE STRING "install directory of Natron, NatronRenderer and natron-python")
set(NATRON_INSTALL_RESOURCESDIR "${_resourcesdir}" CACHE STRING "install directory of the resources (stylesheets, fonts, pixmaps); empty: none")
set(NATRON_INSTALL_PYPLUGSDIR "${_pyplugsdir}" CACHE STRING "install directory of the bundled PyPlugs; empty: none")
set(NATRON_INSTALL_OCIODIR "${_ociodir}" CACHE STRING "install directory of the OpenColorIO configs; empty: none (macOS: they are in Natron.app)")
set(NATRON_INSTALL_DOCDIR "${_docdir}" CACHE STRING "install directory of LICENSE.txt; empty: none")
set(NATRON_INSTALL_TESTSDIR "${_testsdir}" CACHE STRING "install directory of Tests (cmake --install <build> --component Tests)")
# The defaults above depend on NATRON_INSTALL_LAYOUT: when it changes, reset the ones the user didn't set.
if(DEFINED _NATRON_INSTALL_LAYOUT_USED AND NOT _NATRON_INSTALL_LAYOUT_USED STREQUAL NATRON_INSTALL_LAYOUT)
    foreach(_v BUNDLEDIR BINDIR RESOURCESDIR PYPLUGSDIR OCIODIR DOCDIR TESTSDIR)
        if("${NATRON_INSTALL_${_v}}" STREQUAL "${_NATRON_INSTALL_${_v}_DEFAULT}")
            string(TOLOWER "${_v}" _lv)
            string(REGEX REPLACE "dir$" "" _lv "${_lv}")
            set(NATRON_INSTALL_${_v} "${_${_lv}dir}" CACHE STRING "" FORCE)
        endif()
    endforeach()
endif()
foreach(_v BUNDLEDIR BINDIR RESOURCESDIR PYPLUGSDIR OCIODIR DOCDIR TESTSDIR)
    string(TOLOWER "${_v}" _lv)
    string(REGEX REPLACE "dir$" "" _lv "${_lv}")
    set(_NATRON_INSTALL_${_v}_DEFAULT "${_${_lv}dir}" CACHE INTERNAL "")
endforeach()
set(_NATRON_INSTALL_LAYOUT_USED "${NATRON_INSTALL_LAYOUT}" CACHE INTERNAL "")

# Release flavour (qmake: CONFIG+=snapshot/alpha/beta/RC/stable/custombuild, BUILD_NUMBER=N,
# BUILD_USER_NAME=name). Empty means a "Devel" build. Only the build farm should set these.
set(NATRON_BUILD_CONFIG "" CACHE STRING
    "release flavour shown in the version string: empty (Devel), SNAPSHOT, ALPHA, BETA, RC, STABLE or CUSTOM")
set_property(CACHE NATRON_BUILD_CONFIG PROPERTY STRINGS "" SNAPSHOT ALPHA BETA RC STABLE CUSTOM)
# BUILD_NUMBER is the qmake variable (qmake BUILD_NUMBER=N).
if(DEFINED BUILD_NUMBER)
    set(_natron_build_number_default "${BUILD_NUMBER}")
else()
    set(_natron_build_number_default "0")
endif()
set(NATRON_BUILD_NUMBER "${_natron_build_number_default}" CACHE STRING
    "build number of an ALPHA, BETA or RC release (e.g. 2 for RC2)")
# BUILD_USER_NAME is the variable read by earlier versions of the CMake build.
set(NATRON_CUSTOM_BUILD_USER_NAME "${BUILD_USER_NAME}" CACHE STRING
    "for a CUSTOM build, the name shown instead of the version (\"Natron for <name>\")")

# Build options (qmake: CONFIG+=openmp, noassertions, fast, addresssanitizer, threadsanitizer, coverage).
if(NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # libs.pri enables OpenMP by default only with GCC on systems other than macOS.
    set(_natron_openmp_default ON)
else()
    set(_natron_openmp_default OFF)
endif()
# An explicit -DNATRON_USE_OPENMP=ON fails if OpenMP is missing; the default only warns.
if(DEFINED NATRON_USE_OPENMP)
    set(_natron_openmp_explicit ON)
else()
    set(_natron_openmp_explicit OFF)
endif()
option(NATRON_USE_OPENMP
    "use OpenMP in the bundled ceres and openMVG (tracker); with Apple clang, needs libomp (MacPorts: found automatically, else set OpenMP_ROOT)"
    ${_natron_openmp_default})
option(NATRON_NO_ASSERTIONS
    "define NDEBUG, QT_NO_DEBUG, QT_NO_DEBUG_OUTPUT and QT_NO_WARNING_OUTPUT in every configuration, as the installers do"
    OFF)
option(NATRON_FAST_MATH
    "optimize with -O3 -ffast-math (MSVC: /O2 /fp:fast) in every configuration, allowing non-IEEE-conformant floating point, as the installers do"
    OFF)
set(NATRON_SANITIZE "" CACHE STRING
    "sanitizers passed to -fsanitize=, comma-separated (e.g. address, thread, undefined, or address,undefined); use with a Debug or RelWithDebInfo build")
option(NATRON_COVERAGE "instrument for code coverage with --coverage (GCC/Clang); use with a Debug build" OFF)

set(IS_DEBUG_BUILD OFF)
if(CMAKE_BUILD_TYPE MATCHES "^(debug|Debug|DEBUG)$")
    set(IS_DEBUG_BUILD ON)
    add_definitions(-DDEBUG)

    if(WIN32)
        # Debug builds need minimal optimizations to avoid excessive link times and link errors related to Eigen.
        add_compile_options(-O)
    endif()
else()
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
    message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
endif()

find_package(Boost COMPONENTS headers serialization CONFIG)
if(WIN32)
    # Prevent looking in the registry so that the MinGW python can be found.
    set(Python3_FIND_REGISTRY NEVER)
endif()
find_package(Python3 COMPONENTS Interpreter Development)

# RPATH of the installed binaries (CMake drops the build-tree RPATH at install time).
if(NATRON_INSTALL_LAYOUT STREQUAL "PORTABLE")
    # The installers bundle the libraries next to the binaries: only relative paths,
    # so that no build machine path (MacPorts, SDK) is searched on the user's machine.
    # On macOS the PySide2/Shiboken2 libraries are bundled in their site-packages directories,
    # as tools/jenkins/build-OSX-installer.sh does.
    if(APPLE)
        set(_natron_rpath "@loader_path/../Frameworks")
        if(Python3_VERSION_MAJOR AND Python3_VERSION_MINOR)
            set(_natron_pysite "@loader_path/../Frameworks/Python.framework/Versions/${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/lib/python${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}/site-packages")
            # The installer script must bundle the same Python version (its PYVER).
            message(STATUS "PORTABLE install: PySide rpaths use the bundled Python ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} (must match PYVER of the installer script)")
            if(NATRON_QT6)
                list(APPEND _natron_rpath "${_natron_pysite}/shiboken6" "${_natron_pysite}/PySide6")
            else()
                list(APPEND _natron_rpath "${_natron_pysite}/shiboken2" "${_natron_pysite}/PySide2")
            endif()
        endif()
    else()
        set(_natron_rpath "$ORIGIN/../lib")
    endif()
    if(NOT DEFINED CMAKE_INSTALL_RPATH)
        set(CMAKE_INSTALL_RPATH "${_natron_rpath}")
    endif()
    if(NOT DEFINED CMAKE_INSTALL_RPATH_USE_LINK_PATH)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
    endif()
elseif(NOT DEFINED CMAKE_INSTALL_RPATH_USE_LINK_PATH)
    # FHS: installed binaries keep the paths of the libraries they were linked with outside the
    # build tree (Qt, PySide2/Shiboken2, MacPorts libraries), so that they run where they are
    # installed. Pass -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=OFF to install without them.
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

if(IS_DEBUG_BUILD AND WIN32)
    # Explicitly setting SHIBOKEN_PYTHON_LIBRARIES variable to avoid PYTHON_DEBUG_LIBRARY-NOTFOUND
    # link errors on Windows debug builds.
    set(SHIBOKEN_PYTHON_LIBRARIES ${Python3_LIBRARIES})
endif()

set(QT_COMPONENTS Core Gui Network Widgets Concurrent)
if(NATRON_QT6)
    list(APPEND QT_COMPONENTS OpenGLWidgets)
    find_package(Qt6 6.3 CONFIG REQUIRED COMPONENTS ${QT_COMPONENTS})
    find_package(Shiboken6 6.3 CONFIG REQUIRED COMPONENTS libshiboken6)
    find_package(Shiboken6Tools 6.3 CONFIG REQUIRED COMPONENTS shiboken6)
    find_package(PySide6 6.3 CONFIG REQUIRED COMPONENTS pyside6)
    set(QT_VERSION_MAJOR 6)
    set(PYSIDE_VERSION_MAJOR 6)

    get_target_property(PYSIDE_INCLUDE_DIRS PySide6::pyside6 INTERFACE_INCLUDE_DIRECTORIES)
    set(QTCORE_INCLUDE_DIRS ${Qt6Core_INCLUDE_DIRS})
    set(QTGUI_INCLUDE_DIRS ${Qt6Gui_INCLUDE_DIRS})
    set(QTWIDGETS_INCLUDE_DIRS ${Qt6Widgets_INCLUDE_DIRS})

    set(SHIBOKEN_EXE Shiboken6::shiboken6)
    set(SHIBOKEN_LIB Shiboken6::libshiboken)
    set(PYSIDE_LIB PySide6::pyside6)
else()
    find_package(Qt5 5.15 CONFIG REQUIRED COMPONENTS ${QT_COMPONENTS})
    find_package(Shiboken2 5.15 CONFIG QUIET COMPONENTS libshiboken2 shiboken2)
    find_package(PySide2 5.15 CONFIG QUIET COMPONENTS pyside2)
    if(NOT Shiboken2_FOUND AND NOT PySide2_FOUND)
        # No CMake config files (PySide2 built with setup.py, e.g. MacPorts' py*-pyside2,
        # or the download.qt.io wheels): locate them from the Python installation.
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PySide2FromPython.cmake)
        _natron_pyside2_from_python(5.15)
    elseif(NOT Shiboken2_FOUND OR NOT PySide2_FOUND)
        message(FATAL_ERROR "Found only one of the Shiboken2 (${Shiboken2_FOUND}) and PySide2 (${PySide2_FOUND}) "
            "CMake config packages: set Shiboken2_DIR and PySide2_DIR to matching installs.")
    endif()
    set(QT_VERSION_MAJOR 5)
    set(PYSIDE_VERSION_MAJOR 2)

    get_target_property(PYSIDE_INCLUDE_DIRS PySide2::pyside2 INTERFACE_INCLUDE_DIRECTORIES)
    set(QTCORE_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS})
    set(QTGUI_INCLUDE_DIRS ${Qt5Gui_INCLUDE_DIRS})
    set(QTWIDGETS_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})

    set(SHIBOKEN_EXE Shiboken2::shiboken2)
    set(SHIBOKEN_LIB Shiboken2::libshiboken)
    set(PYSIDE_LIB PySide2::pyside2)
endif()

# With Qt frameworks (macOS), the Qt include directories are <prefix>/lib/QtCore.framework[/Headers],
# so the shiboken parser needs -F<prefix>/lib to resolve <QtCore/qnamespace.h> and the like.
# Qt5 only for now: the Qt6 build is not tested with Qt frameworks.
set(SHIBOKEN_QT_FRAMEWORK_ARGS)
if(NOT NATRON_QT6)
    foreach(dir IN LISTS QTCORE_INCLUDE_DIRS QTGUI_INCLUDE_DIRS QTWIDGETS_INCLUDE_DIRS)
        if(dir MATCHES "^(.*)/[^/]+\\.framework(/Headers)?$")
            list(APPEND SHIBOKEN_QT_FRAMEWORK_ARGS "-F${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(SHIBOKEN_QT_FRAMEWORK_ARGS)
        list(REMOVE_DUPLICATES SHIBOKEN_QT_FRAMEWORK_ARGS)
    endif()
endif()

if(IS_DEBUG_BUILD AND WIN32)
    # Remove NDEBUG from Shiboken INTERFACE_COMPILE_DEFINITIONS so it is not inherited in debug builds.
    get_property(SHIBOKEN_DEFS TARGET ${SHIBOKEN_LIB} PROPERTY INTERFACE_COMPILE_DEFINITIONS)
    list(REMOVE_ITEM SHIBOKEN_DEFS NDEBUG)
    set_property(TARGET ${SHIBOKEN_LIB} PROPERTY INTERFACE_COMPILE_DEFINITIONS SHIBOKEN_DEFS)
endif()

# shiboken6 is built with FORCE_LIMITED_API by default, and its imported
# target then passes Py_LIMITED_API on to everything that links it, including
# Natron's own Engine and Gui code, which uses the full CPython API. Natron's
# bindings are embedded modules, so the stable ABI brings nothing: drop the
# definition and build against the full API. Qt6 only, so that Qt5 compile
# flags stay exactly as they were.
if(NATRON_QT6)
    get_property(_shiboken_defs TARGET ${SHIBOKEN_LIB} PROPERTY INTERFACE_COMPILE_DEFINITIONS)
    if(_shiboken_defs)
        list(FILTER _shiboken_defs EXCLUDE REGEX "^Py_LIMITED_API")
        set_property(TARGET ${SHIBOKEN_LIB} PROPERTY INTERFACE_COMPILE_DEFINITIONS ${_shiboken_defs})
    endif()
endif()

#Since in Natron and OpenFX all strings are supposed UTF-8 and that the constructor
#for QString(const char*) assumes ASCII strings, we may run into troubles
add_compile_definitions(QT_NO_CAST_FROM_ASCII)

if (MSVC)
    # Allow __cplusplus to properly reflect the c++ standard version.
    add_compile_options(/Zc:__cplusplus)
    add_compile_definitions(__STDC_LIMIT_MACROS WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
else()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes" )
    add_compile_options(-Wall -Wextra -Wmissing-declarations -Wno-multichar -Winit-self -Wno-long-long
                        -Wvla -Wdate-time -Wno-format-overflow -Wno-deprecated-copy)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wshift-overflow=2 -Wduplicated-cond -Wno-stringop-overflow)
    endif()
endif()

if(NATRON_NO_ASSERTIONS)
    add_compile_definitions(NDEBUG QT_NO_DEBUG QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT)
endif()

if(NATRON_FAST_MATH)
    # Compile options come after CMAKE_<LANG>_FLAGS_<CONFIG> on the command line, so this
    # overrides the -O level of every configuration, like qmake's CONFIG+=fast.
    # -O3 -ffast-math is the documented equivalent of -Ofast, which clang >= 19 deprecates.
    if(MSVC)
        add_compile_options(/O2 /fp:fast)
    else()
        add_compile_options(-O3 -ffast-math)
    endif()
endif()

if(NOT NATRON_SANITIZE STREQUAL "")
    if(MSVC)
        add_compile_options(/fsanitize=${NATRON_SANITIZE})
    else()
        add_compile_options(-fsanitize=${NATRON_SANITIZE} -fno-omit-frame-pointer -fno-optimize-sibling-calls)
        add_link_options(-fsanitize=${NATRON_SANITIZE})
    endif()
endif()

if(NATRON_COVERAGE)
    if(MSVC)
        message(FATAL_ERROR "NATRON_COVERAGE needs GCC or Clang")
    endif()
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

if(NATRON_USE_OPENMP)
    # With Apple clang, FindOpenMP needs omp.h and libomp, which MacPorts installs
    # in non-standard subdirectories (sudo port install libomp).
    if(APPLE AND NOT DEFINED OpenMP_ROOT AND NOT DEFINED ENV{OpenMP_ROOT}
       AND EXISTS "/opt/local/lib/libomp/libomp.dylib")
        set(OpenMP_ROOT "/opt/local/include/libomp;/opt/local/lib/libomp")
    endif()
    find_package(OpenMP COMPONENTS CXX)
    if(NOT OpenMP_CXX_FOUND AND _natron_openmp_explicit)
        message(FATAL_ERROR "NATRON_USE_OPENMP is ON but OpenMP was not found. "
            "Install libomp (or your compiler's OpenMP runtime) and set OpenMP_ROOT to its prefix, or configure with -DNATRON_USE_OPENMP=OFF.")
    elseif(NOT OpenMP_CXX_FOUND)
        message(WARNING "OpenMP was not found: building ceres and openMVG without it (NATRON_USE_OPENMP=OFF)")
        set(NATRON_USE_OPENMP OFF CACHE BOOL "" FORCE)
    endif()
endif()

if(NATRON_SYSTEM_LIBS)
    find_package(glog)
    find_package(Ceres)
endif()

if(UNIX AND NOT APPLE)
    find_package(ECM NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
    find_package(X11 REQUIRED)
    find_package(Wayland COMPONENTS Client Egl)
elseif(APPLE)
    enable_language(OBJCXX)
elseif(WIN32)
    enable_language(RC)
    add_compile_definitions(WINDOWS WIN32 _UNICODE UNICODE NOMINMAX QHTTP_SERVER_STATIC)
    if (NOT MSVC)
        add_compile_options(-municode)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # compile and link, like qmake's win32-g++ mkspec
            add_compile_options(-mthreads)
            add_link_options(-mthreads)
        endif()
        # Only Natron itself is a GUI program (WIN32_EXECUTABLE in App/CMakeLists.txt,
        # which adds -mwindows). NatronRenderer, natron-python and Tests are
        # console programs, as with qmake (CONFIG += console in their .pro files).
        add_link_options(-municode)
        set(CMAKE_RC_COMPILER_INIT windres)
        set(CMAKE_RC_COMPILE_OBJECT
            "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

enable_testing()

add_subdirectory(libs)

# HostSupport: openfx's OfxHost target (openfx::HostSupport), with Natron's options and the
# parametric parameters of libs/OpenFX_extensions. It links ${CMAKE_DL_LIBS} itself (dlopen in
# ofxhBinary.cpp). HostSupport/HostSupport.pro is the qmake equivalent: keep them in sync.
# These normal variables intentionally force openfx's options: a user's -DOFX_*=... has no effect.
set(OFX_BUILD_HOSTSUPPORT ON)
set(OFX_EXTENSIONS_NATRON ON)
set(OFX_EXTENSIONS_NUKE ON)
set(OFX_EXTENSIONS_TUTTLE ON)
set(OFX_EXTENSIONS_VEGAS ON)
set(OFX_EXTENSIONS_RESOLVE ON)
set(OFX_SUPPORTS_OPENGLRENDER ON)
set(OFX_SUPPORTS_MULTITHREAD ON)
set(OFX_SUPPORTS_DIALOG ON)
set(OFX_SUPPORTS_PARAMETRIC ON)
add_subdirectory(libs/OpenFX EXCLUDE_FROM_ALL)
target_sources(OfxHost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libs/OpenFX_extensions/ofxhParametricParam.cpp)
target_include_directories(OfxHost PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/OpenFX_extensions>)
add_library(HostSupport ALIAS OfxHost)

# Release flavour, read by Global/Macros.h. Set after the bundled libs and
# HostSupport so that changing the build number does not rebuild them.
string(TOUPPER "${NATRON_BUILD_CONFIG}" _natron_build_config)
if(NOT NATRON_BUILD_NUMBER MATCHES "^[0-9]+$")
    message(FATAL_ERROR "NATRON_BUILD_NUMBER must be a non-negative integer, not '${NATRON_BUILD_NUMBER}'")
endif()
if(_natron_build_config STREQUAL "")
    # Devel build
elseif(_natron_build_config MATCHES "^(SNAPSHOT|ALPHA|BETA|RC|STABLE)$")
    add_compile_definitions(NATRON_CONFIG_${_natron_build_config})
elseif(_natron_build_config STREQUAL "CUSTOM")
    # Like tools/jenkins/build-natron.sh: a stable build that shows the user name.
    if(NATRON_CUSTOM_BUILD_USER_NAME STREQUAL "")
        message(FATAL_ERROR "NATRON_BUILD_CONFIG=CUSTOM needs NATRON_CUSTOM_BUILD_USER_NAME")
    endif()
    add_compile_definitions(NATRON_CONFIG_STABLE)
else()
    message(FATAL_ERROR "Unknown NATRON_BUILD_CONFIG '${NATRON_BUILD_CONFIG}': "
        "use an empty value (Devel), SNAPSHOT, ALPHA, BETA, RC, STABLE or CUSTOM")
endif()
if(NOT _natron_build_config STREQUAL "")
    message(STATUS "Natron release flavour: ${_natron_build_config}, build number ${NATRON_BUILD_NUMBER}")
endif()
add_compile_definitions(NATRON_BUILD_NUMBER=${NATRON_BUILD_NUMBER})
if(_natron_build_config STREQUAL "CUSTOM")
    # Only for a CUSTOM build, as qmake's CONFIG+=custombuild: Global/Macros.h stringizes the
    # token, so it must be the bare name. Quotes, or an empty token, would end up in the
    # name shown by the splash screen and the About window (e.g. 'Natron for ""').
    add_compile_definitions(NATRON_CUSTOM_BUILD_USER_TOKEN=${NATRON_CUSTOM_BUILD_USER_NAME})
elseif(NOT NATRON_CUSTOM_BUILD_USER_NAME STREQUAL "")
    message(WARNING "NATRON_CUSTOM_BUILD_USER_NAME is only used with NATRON_BUILD_CONFIG=CUSTOM: ignored")
endif()


add_subdirectory(Engine)

add_subdirectory(PythonBin)

add_subdirectory(Renderer)

add_subdirectory(Gui)

if(NATRON_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

add_subdirectory(App)
