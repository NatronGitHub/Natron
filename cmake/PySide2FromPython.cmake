# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
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

# Locate Shiboken2 and PySide2 from the Python installation, for installs that
# have no CMake config files (Shiboken2Config.cmake, PySide2Config.cmake):
# PySide2 built with setup.py (MacPorts py*-pyside2) or installed from the
# PyPI wheels (PySide2 + shiboken2 + shiboken2_generator).
#
# The layout of these installs is, in the site-packages of Python3_EXECUTABLE:
#   shiboken2/                libshiboken2*.{dylib,so*} (shiboken2*.lib/.dll on Windows)
#   shiboken2_generator/      shiboken2 (the generator), include/ (libshiboken headers)
#   PySide2/                  libpyside2*, include/, typesystems/, _config.py
#
# This defines the same names as the CMake config packages, as used by Natron:
#   Shiboken2::libshiboken    IMPORTED shared library
#   Shiboken2::shiboken2      IMPORTED executable (the generator)
#   PySide2::pyside2          IMPORTED shared library
#   PYSIDE_TYPESYSTEMS        directory of the PySide2 typesystem files
# and sets Shiboken2_FOUND, PySide2_FOUND, PySide2_VERSION.
#
# The generator can be overridden with -DSHIBOKEN2_GENERATOR=/path/to/shiboken2.

function(_natron_pyside2_glob out_var dir)
    set(found)
    foreach(pattern IN LISTS ARGN)
        file(GLOB candidates "${dir}/${pattern}")
        list(SORT candidates)
        foreach(c IN LISTS candidates)
            # skip Python extension modules (shiboken2.cpython-310-darwin.so)
            get_filename_component(name "${c}" NAME)
            if(NOT IS_DIRECTORY "${c}" AND NOT name MATCHES "^(shiboken2|Qt[A-Za-z0-9]*)\\.(cpython|abi3|pypy)")
                set(found "${c}")
                break()
            endif()
        endforeach()
        if(found)
            break()
        endif()
    endforeach()
    set(${out_var} "${found}" PARENT_SCOPE)
endfunction()

function(_natron_pyside2_from_python min_version)
    if(NOT Python3_EXECUTABLE)
        message(FATAL_ERROR "Shiboken2/PySide2: no Python3 interpreter was found, cannot locate them.")
    endif()

    # Locate the packages without importing them (importing PySide2 loads Qt).
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c
            "import importlib.util as u\nfor m in ('shiboken2', 'shiboken2_generator', 'PySide2'):\n    s = u.find_spec(m)\n    print(s.submodule_search_locations[0] if s and s.submodule_search_locations else '-')"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Shiboken2/PySide2: ${Python3_EXECUTABLE} failed to look for them:\n${err}")
    endif()
    string(REPLACE "\\" "/" out "${out}")
    string(REPLACE "\n" ";" out "${out}")
    list(LENGTH out n)
    if(NOT n EQUAL 3)
        message(FATAL_ERROR "Shiboken2/PySide2: unexpected output from ${Python3_EXECUTABLE}: ${out}")
    endif()
    list(GET out 0 shiboken_dir)
    list(GET out 1 generator_dir)
    list(GET out 2 pyside_dir)
    foreach(v shiboken_dir generator_dir pyside_dir)
        if(${v} STREQUAL "-")
            set(${v})
        endif()
    endforeach()

    set(missing)
    if(NOT shiboken_dir)
        list(APPEND missing "the shiboken2 module")
    endif()
    if(NOT pyside_dir)
        list(APPEND missing "the PySide2 module")
    endif()

    # Version, from PySide2/_config.py (written by setup.py)
    set(version)
    if(pyside_dir AND EXISTS "${pyside_dir}/_config.py")
        file(STRINGS "${pyside_dir}/_config.py" version_line REGEX "^version = ")
        string(REGEX REPLACE "^version = \"([^\"]*)\".*" "\\1" version "${version_line}")
    endif()

    # Libraries
    if(WIN32)
        set(dll_patterns "shiboken2*.dll")
        set(lib_patterns "shiboken2*.lib")
        set(pyside_dll_patterns "pyside2*.dll")
        set(pyside_lib_patterns "pyside2*.lib")
    elseif(APPLE)
        set(lib_patterns "libshiboken2*.dylib")
        set(pyside_lib_patterns "libpyside2*.dylib")
    else()
        set(lib_patterns "libshiboken2*.so*")
        set(pyside_lib_patterns "libpyside2*.so*")
    endif()
    if(shiboken_dir)
        _natron_pyside2_glob(shiboken_lib "${shiboken_dir}" ${lib_patterns})
        if(NOT shiboken_lib)
            list(APPEND missing "libshiboken2 in ${shiboken_dir}")
        endif()
        if(WIN32)
            _natron_pyside2_glob(shiboken_dll "${shiboken_dir}" ${dll_patterns})
        endif()
    endif()
    if(pyside_dir)
        _natron_pyside2_glob(pyside_lib "${pyside_dir}" ${pyside_lib_patterns})
        if(NOT pyside_lib)
            list(APPEND missing "libpyside2 in ${pyside_dir}")
        endif()
        if(WIN32)
            _natron_pyside2_glob(pyside_dll "${pyside_dir}" ${pyside_dll_patterns})
        endif()
        if(NOT EXISTS "${pyside_dir}/include/pyside.h")
            list(APPEND missing "the PySide2 headers (${pyside_dir}/include)")
        endif()
        if(NOT EXISTS "${pyside_dir}/typesystems/typesystem_core.xml")
            list(APPEND missing "the PySide2 typesystems (${pyside_dir}/typesystems)")
        endif()
    endif()

    # The shiboken2 generator and the libshiboken headers come from shiboken2_generator.
    # Prefer the binary in shiboken2_generator over the bin/ wrapper scripts.
    set(generator_doc "shiboken2 generator, used when Shiboken2 has no CMake config files")
    if(generator_dir)
        find_program(SHIBOKEN2_GENERATOR NAMES shiboken2 PATHS "${generator_dir}"
            NO_DEFAULT_PATH DOC "${generator_doc}")
    endif()
    find_program(SHIBOKEN2_GENERATOR NAMES shiboken2 DOC "${generator_doc}")
    if(NOT SHIBOKEN2_GENERATOR)
        list(APPEND missing "the shiboken2 generator (shiboken2_generator module, or set SHIBOKEN2_GENERATOR)")
    endif()
    set(shiboken_include)
    foreach(d "${generator_dir}/include" "${shiboken_dir}/include")
        if(EXISTS "${d}/shiboken.h")
            set(shiboken_include "${d}")
            break()
        endif()
    endforeach()
    if(NOT shiboken_include)
        list(APPEND missing "the libshiboken2 headers (shiboken2_generator/include)")
    endif()

    if(missing)
        list(JOIN missing ", " missing)
        message(FATAL_ERROR
            "Shiboken2/PySide2 ${min_version} not found: there are no CMake config files "
            "(set Shiboken2_DIR and PySide2_DIR if you have them), and ${Python3_EXECUTABLE} "
            "does not provide: ${missing}.")
    endif()
    if(version AND version VERSION_LESS min_version)
        message(FATAL_ERROR "PySide2 ${version} found in ${pyside_dir}, but ${min_version} or later is required.")
    endif()

    add_library(Shiboken2::libshiboken SHARED IMPORTED GLOBAL)
    set_target_properties(Shiboken2::libshiboken PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${shiboken_include}")
    add_executable(Shiboken2::shiboken2 IMPORTED GLOBAL)
    set_target_properties(Shiboken2::shiboken2 PROPERTIES IMPORTED_LOCATION "${SHIBOKEN2_GENERATOR}")
    add_library(PySide2::pyside2 SHARED IMPORTED GLOBAL)
    set_target_properties(PySide2::pyside2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${pyside_dir}/include"
        INTERFACE_LINK_LIBRARIES Shiboken2::libshiboken)
    if(WIN32)
        set_target_properties(Shiboken2::libshiboken PROPERTIES
            IMPORTED_IMPLIB "${shiboken_lib}" IMPORTED_LOCATION "${shiboken_dll}")
        set_target_properties(PySide2::pyside2 PROPERTIES
            IMPORTED_IMPLIB "${pyside_lib}" IMPORTED_LOCATION "${pyside_dll}")
    else()
        set_target_properties(Shiboken2::libshiboken PROPERTIES IMPORTED_LOCATION "${shiboken_lib}")
        set_target_properties(PySide2::pyside2 PROPERTIES IMPORTED_LOCATION "${pyside_lib}")
    endif()

    message(STATUS "Found Shiboken2/PySide2 ${version} without CMake config files, from ${Python3_EXECUTABLE}:")
    message(STATUS "  libshiboken2: ${shiboken_lib}")
    message(STATUS "  shiboken2 generator: ${SHIBOKEN2_GENERATOR}")
    message(STATUS "  libpyside2: ${pyside_lib}")

    set(Shiboken2_FOUND TRUE PARENT_SCOPE)
    set(PySide2_FOUND TRUE PARENT_SCOPE)
    set(PySide2_VERSION "${version}" PARENT_SCOPE)
    set(PYSIDE_TYPESYSTEMS "${pyside_dir}/typesystems" PARENT_SCOPE)
endfunction()
