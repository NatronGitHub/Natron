#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
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
#
# Build the portable archive for Linux
#
# Options:
# DISABLE_BREAKPAD=1: Disable automatic crash report (always set: the Linux SDK has no Breakpad)


set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

echo "*** Linux installer..."

source common.sh
source manageBuildOptions.sh
source manageLog.sh
updateBuildOptions

# The Linux SDK no longer builds Breakpad (dump_syms, crash reporter), see #1183
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    echo "Warning: the Linux SDK has no Breakpad (dump_syms): packaging without the crash reporter (DISABLE_BREAKPAD=1)."
    DISABLE_BREAKPAD=1
fi

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

# (MAGICK7_PREFIX: openfx-arena's ImageMagick 7, see common.sh; fixrpath removes the plug-in's
# build rpath, so ldd only finds the libraries to bundle through LD_LIBRARY_PATH)
# The C++ runtime is not taken from the SDK: see SYSTEM_LIBS_RE below.
LD_LIBRARY_PATH="${SDK_HOME}/lib:${FFMPEG_PATH}/lib:${SDK_HOME}/qt${QT_VERSION_MAJOR}/lib${MAGICK7_PREFIX:+:${MAGICK7_PREFIX}/lib}"
PATH="${SDK_HOME}/bin:$PATH"
export C_INCLUDE_PATH="${SDK_HOME}/include:${SDK_HOME}/qt${QT_VERSION_MAJOR}/include"
export CPLUS_INCLUDE_PATH="${C_INCLUDE_PATH}"
export LD_LIBRARY_PATH

# Libraries that always come from the system and are never bundled. A bundled file may need
# (DT_NEEDED) only these libraries and the bundled ones (see checkBundle):
# - glibc (libc, libm, libdl, libpthread, librt, ld-linux*, libnsl, libnss_*, libBrokenLocale,
#   libc_malloc_debug, ...);
# - the GCC runtime (libstdc++, libgcc_s, libgomp, libatomic). Natron is built on Rocky Linux 9
#   with gcc-toolset: its libstdc++.so is a linker script that takes the symbols missing
#   from the system libstdc++ (GCC 11) from libstdc++_nonshared.a, so the binaries only need
#   the system libstdc++;
# - OpenGL (GLVND: libGL, libEGL, libGLX, libGLdispatch, libOpenGL, libGLESv1_CM, libGLESv2),
#   libdrm and libdrm_*, and Mesa's libgbm and libglapi, which must match the system's
#   graphics driver (a bundled older copy breaks the system's Mesa drivers);
# - systemd's libudev (it reads the running udev's private database, whose format follows the
#   system's udev) and libselinux;
# - the GPU drivers' libraries: libcuda, NVIDIA's libnvidia-* and libnvcuvid, and the OpenCL
#   vendor ICDs (libamdocl*, libigdrcl, libintelocl, libMesaOpenCL, libRusticlOpenCL, libpocl):
#   the OpenCL ICD loader only opens the ICDs named in /etc/OpenCL/vendors/*.icd. The ICD loader
#   itself, libOpenCL.so.1, is not a system library (a default EL9 or Ubuntu install doesn't
#   have it): it is bundled if a bundled file needs it;
# - the desktop libraries, which every Linux desktop has and which must match the system's
#   display server and sound system: the X11 client libraries (libX11, libX11-xcb, libXext,
#   libXrender, libXau, libXdmcp, libXi, libXfixes, ... i.e. libX*), libxcb and its extensions,
#   the xcb-util libraries (libxcb-icccm, -image, -keysyms, -render-util...), libSM, libICE,
#   libxkbcommon and libxkbcommon-x11, and ALSA (libasound).
# INSTALL_LINUX.md lists them ("Runtime requirements of the Linux binaries"). The OpenGL, DRM, udev,
# SELinux and GPU driver libraries are the same as in openfx's Support/include/linuxCheckBundle.sh.
# Unlike there, libcrypt is not a system library: Debian and Ubuntu don't have libcrypt.so.2, so a
# bundled file must not need it (Python's crypt module is left out, see INSTALL_LINUX.md).
# A library in this list that would be bundled is an error.
SYSTEM_LIBS_RE='^(ld-linux.*|lib(c|m|mvec|dl|pthread|rt|resolv|util|anl|thread_db|nsl|nss_.*|BrokenLocale|c_malloc_debug|stdc\+\+|gcc_s|gomp|atomic|GL|EGL|GLX|GLdispatch|OpenGL|GLESv1_CM|GLESv2|drm|drm_.*|gbm|glapi|udev|selinux|cuda|nvidia-.*|nvcuvid|amdocl.*|igdrcl|intelocl|MesaOpenCL|RusticlOpenCL|pocl|X[A-Za-z0-9]*|X11-xcb|xcb|xcb-.*|SM|ICE|xkbcommon|xkbcommon-x11|asound)\.so(\..*)?)$'

# Highest symbol versions defined by the system libraries of Rocky Linux 9.0 (glibc 2.34;
# libstdc++, libgcc_s and libgomp from GCC 11), on x86_64 and aarch64. A bundled binary
# that needs a newer version would not run on the oldest supported systems.
# Later EL9 glibc packages also define some GLIBC_2.35 symbols (backports, e.g. __memcmpeq,
# _dl_find_object): a binary that uses them would not run on EL 9.0 to 9.x, hence GLIBC_2.34.
# libgcc_s: the newest version of GCC 11's libgcc_s is GCC_7.0.0 on x86_64 and GCC_11.0 on
# aarch64 (as in openfx's Support/include/linuxCheckBundle.sh).
if [ "${ARCH:-$(uname -m)}" = "aarch64" ]; then
    SYSTEM_SYMVER_GCC_MAX=GCC_11.0
else
    SYSTEM_SYMVER_GCC_MAX=GCC_7.0.0
fi
SYSTEM_SYMVER_MAX="GLIBC_2.34 GLIBCXX_3.4.29 CXXABI_1.3.13 ${SYSTEM_SYMVER_GCC_MAX} GOMP_5.0.1 OMP_5.0.1"
# Versions of these libraries without a number that EL9 has (libstdc++'s transactional memory
# and __float128 ABIs). Any other one, e.g. GLIBC_PRIVATE, or GLIBC_ABI_DT_RELR (glibc >= 2.36,
# from -z pack-relative-relocs), is an error.
SYSTEM_SYMVER_OTHER="CXXABI_TM_1 CXXABI_FLOAT128"

function isSystemLib() {
    [[ "$(basename "$1")" =~ $SYSTEM_LIBS_RE ]]
}

# Print the SDK libraries needed by the given ELF files, i.e. the libraries to bundle.
# Fails if one of them is a system library.
function sdkDepends() {
    local deps d
    deps="$(ldd "$@" 2>/dev/null | grep "$SDK_HOME" | awk '{print $3}' | sort | uniq)" || true
    for d in $deps; do
        if isSystemLib "$d"; then
            (>&2 echo "Error: $d is a system library and must not be bundled. Check LD_LIBRARY_PATH and the SDK.")
            exit 1
        fi
        echo "$d"
    done
}

# Prints the problem, if the symbol version $1 (e.g. GLIBCXX_3.4.29) of a system library is not
# available on the oldest supported system. Versions of the other libraries are not checked.
function symverProblem() {
    local v="$1" m prefix num max
    for m in $SYSTEM_SYMVER_OTHER; do
        [ "$v" = "$m" ] && return 0
    done
    for m in $SYSTEM_SYMVER_MAX; do
        prefix="${m%_*}"
        max="${m##*_}"
        [[ "$v" == "${prefix}_"* ]] || continue
        num="${v#"${prefix}_"}"
        if ! [[ "$num" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
            echo "needs $v, which the system libraries of Rocky Linux 9 don't define"
        elif [ "$num" != "$max" ] && [ "$(printf '%s\n%s\n' "$max" "$num" | sort -V | tail -n 1)" = "$num" ]; then
            echo "needs $v, newer than the system libraries of Rocky Linux 9 ($SYSTEM_SYMVER_MAX)"
        fi
        return 0
    done
}

# Check a bundle directory:
# - it must exist and contain ELF files;
# - it must contain no system library;
# - every library that an ELF file in it needs (DT_NEEDED) must be either bundled (a file or
#   symlink with that name anywhere in the directory) or a system library (SYSTEM_LIBS_RE);
# - no ELF file in it may need a symbol version of a system library that is newer than
#   SYSTEM_SYMVER_MAX, or a version without a number (except SYSTEM_SYMVER_OTHER).
# All the problems are listed, then it fails if there is any.
function checkBundle() {
    local dir="$1" f v t lib kind problem errors=0 elfs=0
    local -A bundled=()
    for t in find od objdump sort; do
        if ! command -v "$t" > /dev/null; then
            (>&2 echo "Error: $t is required to check the bundle")
            exit 1
        fi
    done
    if [ ! -d "$dir" ]; then
        (>&2 echo "Error: $dir: no such directory, nothing to check")
        exit 1
    fi
    while IFS= read -r -d '' f; do
        bundled["$(basename "$f")"]=1
        if isSystemLib "$f"; then
            (>&2 echo "Error: system library bundled: $f")
            errors=$((errors + 1))
        fi
    done < <(find "$dir" \( -type f -o -type l \) -name 'lib*.so*' -print0)
    while IFS= read -r -d '' f; do
        [ "$(od -An -tx1 -N4 "$f" | tr -d ' \n')" = "7f454c46" ] || continue
        elfs=$((elfs + 1))
        # "N <lib>" for each DT_NEEDED, "V <lib> <version>" for each version reference (binutils
        # 2.35 on EL9 prints "private flags = 0x0:" right after them, hence the strict parsing)
        while read -r kind lib v; do
            if [ "$kind" = N ]; then
                if [ -z "${bundled[$lib]+x}" ] && ! isSystemLib "$lib"; then
                    (>&2 echo "Error: $f needs $lib, which is neither bundled nor a system library (see SYSTEM_LIBS_RE)")
                    errors=$((errors + 1))
                fi
            elif isSystemLib "$lib"; then
                problem="$(symverProblem "$v")"
                if [ -n "$problem" ]; then
                    (>&2 echo "Error: $f $problem (from $lib)")
                    errors=$((errors + 1))
                fi
            fi
        done < <(objdump -p "$f" 2>/dev/null | awk '
            /^Dynamic Section:/ {d=1; next}
            d && $1 == "NEEDED" {print "N", $2; next}
            d && NF == 0 {d=0}
            /^Version References:/ {r=1; next}
            r && /^ *required from / {lib=$3; sub(/:$/, "", lib); next}
            r && $1 ~ /^0x/ && NF == 4 {print "V", lib, $4; next}
            r {r=0}' | sort -u)
    done < <(find "$dir" -type f \( -name '*.so' -o -name '*.so.*' -o -name '*.ofx' -o -perm -u+x \) -print0)
    if [ "$elfs" -eq 0 ]; then
        (>&2 echo "Error: $dir contains no ELF file: wrong directory?")
        errors=$((errors + 1))
    fi
    if [ "$errors" -gt 0 ]; then
        (>&2 echo "Error: $errors problem(s) with the system libraries in $dir, see above")
        exit 1
    fi
    echo "* $dir: $elfs ELF files; no system library bundled, every needed library is bundled or a system library, no symbol version newer than $SYSTEM_SYMVER_MAX"
}

# Print the system libraries (SYSTEM_LIBS_RE) needed by the ELF files in a directory.
function systemLibsNeeded() {
    local f lib
    while IFS= read -r -d '' f; do
        [ "$(od -An -tx1 -N4 "$f" | tr -d ' \n')" = "7f454c46" ] || continue
        objdump -p "$f" 2>/dev/null | awk '$1 == "NEEDED" {print $2}'
    done < <(find "$1" -type f \( -name '*.so' -o -name '*.so.*' -o -name '*.ofx' \) -print0) | sort -u | while read -r lib; do
        if isSystemLib "$lib"; then
            echo "$lib"
        fi
    done
}

# Pretty architecture/platform identifier (used for breakpad symbol files)
case "${ARCH}" in
    aarch64) PKGOS_BITS="${PKGOS}-aarch64" ;;
    *) PKGOS_BITS="${PKGOS}-x86_${BITS}bit" ;;
esac

if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi

mkdir -p "${BUILD_ARCHIVE_DIRECTORY}"
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mkdir "${BUILD_ARCHIVE_DIRECTORY}/symbols"
fi

# If we are in DEBUG_SYMBOLS mode there might already be deployed installer, remove them
if [ ! -z "$DEBUG_SCRIPTS" ]; then
    (cd "${TMP_BINARIES_PATH}" ; find . -type d -name 'Natron-*' -exec rm -rf {} \;) || true
fi

mkdir -p "${TMP_PORTABLE_DIR}"



if [ -z "${NATRON_BUILD_CONFIG:-}" ]; then
    echo "NATRON_BUILD_CONFIG empty"
    exit 1
fi

# tag symbols we want to keep with 'release'
if [ "${NATRON_BUILD_CONFIG}" = "RELEASE" ] || [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    BPAD_TAG="-release"
fi


BINARIES_TO_INSTALL=""
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
mv "${TMP_BINARIES_PATH}/bin/Natron" "${TMP_BINARIES_PATH}/bin/Natron-bin"
mv "${TMP_BINARIES_PATH}/bin/NatronRenderer" "${TMP_BINARIES_PATH}/bin/NatronRenderer-bin"
mv "${TMP_BINARIES_PATH}/bin/NatronCrashReporter" "${TMP_BINARIES_PATH}/bin/Natron"
mv "${TMP_BINARIES_PATH}/bin/NatronRendererCrashReporter" "${TMP_BINARIES_PATH}/bin/NatronRenderer"
BINARIES_TO_INSTALL="${TMP_BINARIES_PATH}/bin/Natron ${TMP_BINARIES_PATH}/bin/NatronRenderer ${TMP_BINARIES_PATH}/bin/Natron-bin ${TMP_BINARIES_PATH}/bin/NatronRenderer-bin"
else
BINARIES_TO_INSTALL="${TMP_BINARIES_PATH}/bin/Natron ${TMP_BINARIES_PATH}/bin/NatronRenderer"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/NatronProjectConverter" ]; then
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${TMP_BINARIES_PATH}/bin/NatronProjectConverter"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/natron-python" ]; then
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${TMP_BINARIES_PATH}/bin/natron-python"
fi

BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${SDK_HOME}/bin/iconvert ${SDK_HOME}/bin/idiff ${SDK_HOME}/bin/igrep ${SDK_HOME}/bin/iinfo"
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${SDK_HOME}/bin/exrheader ${SDK_HOME}/bin/tiffinfo"
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${FFMPEG_PATH}/bin/ffmpeg ${FFMPEG_PATH}/bin/ffprobe"

mkdir -p "${TMP_PORTABLE_DIR}/docs" "${TMP_PORTABLE_DIR}/bin" "${TMP_PORTABLE_DIR}/Resources" "${TMP_PORTABLE_DIR}/Plugins/PyPlugs" "${TMP_PORTABLE_DIR}/Resources/stylesheets"
cp -pPR "${TMP_BINARIES_PATH}/docs/natron"/* "${TMP_PORTABLE_DIR}/docs/"
cp -R "${TMP_BINARIES_PATH}/Resources/etc"  "${TMP_PORTABLE_DIR}/Resources/"
cp "${TMP_BINARIES_PATH}/Resources/stylesheets"/mainstyle.qss "${TMP_PORTABLE_DIR}/Resources/stylesheets/"
cp "$INC_PATH/natron/natron-mime.sh" "${TMP_PORTABLE_DIR}/bin/"
cp -a "${TMP_BINARIES_PATH}/PyPlugs"/* "${TMP_PORTABLE_DIR}/Plugins/PyPlugs/"
# a Python module: Plugins/ is on the Python path
cp "${TMP_BINARIES_PATH}/natron_convert_project.py" "${TMP_PORTABLE_DIR}/Plugins/"

# Launch scripts
cp "$INC_PATH/scripts/Natron-Linux.sh" "${TMP_PORTABLE_DIR}/Natron"
$GSED -e "s#bin/Natron#bin/NatronRenderer#" "$INC_PATH/scripts/Natron-Linux.sh" > "${TMP_PORTABLE_DIR}/NatronRenderer"
chmod a+x "${TMP_PORTABLE_DIR}/Natron" "${TMP_PORTABLE_DIR}/NatronRenderer"

# Include the test program that we will use later on (removed after the tests)
cp "${TMP_BINARIES_PATH}/bin/Tests" "${TMP_PORTABLE_DIR}/bin/"


for b in $BINARIES_TO_INSTALL; do
    cp "$b" "${TMP_PORTABLE_DIR}/bin/"
done

# If the binary contains upper case letters, make a symbolic link.
# Solves https://github.com/NatronGitHub/Natron/issues/225
for b in Natron NatronRenderer NatronProjectConverter; do
    if [ -f "${TMP_PORTABLE_DIR}/bin/$b" ]; then
        fname="$(basename "$b")"
        fnamel="$(echo "$fname" | tr '[:upper:]' '[:lower:]')" # https://stackoverflow.com/a/2264537
        if [ "$fname" != "$fnamel" ]; then
            ln -s "$fname" "${TMP_PORTABLE_DIR}/bin/$fnamel"
        fi
    fi
done

# Dump symbols for crash reporting
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    "${SDK_HOME}/bin"/dump_syms "${TMP_BINARIES_PATH}/bin/Natron" > "${BUILD_ARCHIVE_DIRECTORY}/symbols/Natron-${CURRENT_DATE}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    "${SDK_HOME}/bin"/dump_syms "${TMP_BINARIES_PATH}/bin/NatronRenderer" > "${BUILD_ARCHIVE_DIRECTORY}/symbols/NatronRenderer-${CURRENT_DATE}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
fi

# Get all dependencies of the binaries
CORE_DEPENDS="$(sdkDepends $(find "${TMP_PORTABLE_DIR}/bin" -maxdepth 1 -type f))"

# icu libraries don't seem to be picked up by this ldd call above
pushd "${SDK_HOME}/lib"
for dep in {libicudata.so.*,libicui18n.so.*,libicuuc.so.*,libbz2.so.*,liblcms2.so.*,libcairo.so.*,libOpenColorIO.so.*}; do
    if [ -f "$dep" ]; then
        CORE_DEPENDS="$CORE_DEPENDS ${SDK_HOME}/lib/$dep"
    fi
done
popd


# OCIO configs
mkdir -p "${TMP_PORTABLE_DIR}/Resources"
cp -LR "${TMP_BINARIES_PATH}/Resources/OpenColorIO-Configs" "${TMP_PORTABLE_DIR}/Resources/"


# Function to fix rpath of libraries in a list of folders
function fixrpath() {
    FOLDERS="$1"
    RPATH="$2"

    for folder in $FOLDERS; do
        (cd "$folder";
        for i in *; do
            if [ -f "$i" ]; then
                chmod u+w $i
                patchelf --force-rpath --set-rpath "\$ORIGIN${RPATH}" "$i" || true
                optlibs="$(env LD_LIBRARY_PATH="" ldd "$i" | grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)"
                if [ ! -z "$optlibs" ]; then
                    for r in $optlibs; do
                        echo "Warning: runtime path remaining to $r for $folder/$i"
                    done
                fi
            fi
        done
    )
    done
}

mkdir -p "${TMP_PORTABLE_DIR}/bin" "${TMP_PORTABLE_DIR}/lib" "${TMP_PORTABLE_DIR}/Resources/pixmaps"
cp "${TMP_BINARIES_PATH}/Resources/pixmaps/natronIcon256_linux.png" "${TMP_PORTABLE_DIR}/Resources/pixmaps/"
cp "${TMP_BINARIES_PATH}/Resources/pixmaps/natronProjectIcon_linux.png" "${TMP_PORTABLE_DIR}/Resources/pixmaps/"
cp -pPR "${SDK_HOME}/share/poppler" "${TMP_PORTABLE_DIR}/Resources/"
cp -pPR "${SDK_HOME}/qt${QT_VERSION_MAJOR}/plugins"/* "${TMP_PORTABLE_DIR}/bin/"

# Copy dependencies
for i in $CORE_DEPENDS; do
    dep=$(basename "$i")
    if [ ! -f "${TMP_PORTABLE_DIR}/lib/$dep" ]; then
        cp -f "$i" "${TMP_PORTABLE_DIR}/lib/"
    fi
done

# Copy dependencies of the libraries
LIB_DEPENDS=$(sdkDepends $(find "${TMP_PORTABLE_DIR}/lib" -maxdepth 1 -type f))
for y in $LIB_DEPENDS; do
    dep=$(basename "$y")
    if [ ! -f "${TMP_PORTABLE_DIR}/lib/$dep" ]; then
        cp -f "$y" "${TMP_PORTABLE_DIR}/lib/"
    fi
done

# Qt plug-in dependencies
QT_PLUG_DEPENDS=$(sdkDepends $(find "${TMP_PORTABLE_DIR}/bin" -maxdepth 2 -type f -name '*.so'))
for z in $QT_PLUG_DEPENDS; do
    dep=$(basename "$z")
    if [ ! -f "${TMP_PORTABLE_DIR}/lib/$dep" ]; then
        cp -f "$z" "${TMP_PORTABLE_DIR}/lib/"
    fi
done


# done in build-natron.sh
#mkdir -p "${TMP_PORTABLE_DIR}/Resources/etc/fonts/conf.d"
#cp "${SDK_HOME}/etc/fonts/fonts.conf" "${TMP_PORTABLE_DIR}/Resources/etc/fonts/"
#cp "${SDK_HOME}/share/fontconfig/conf.avail"/* "${TMP_PORTABLE_DIR}/Resources/etc/fonts/conf.d/"
#$GSED -i "s#${SDK_HOME}/#/#;/conf.d/d" "${TMP_PORTABLE_DIR}/Resources/etc/fonts/fonts.conf"

# strip binaries
if [ "$COMPILE_TYPE" != "debug" ]; then
    strip -s "${TMP_PORTABLE_DIR}/bin"/* &>/dev/null || true
    strip -s "${TMP_PORTABLE_DIR}/lib"/* &>/dev/null || true
    strip -s "${TMP_PORTABLE_DIR}/bin"/*/* &>/dev/null || true
fi

if [ ! -d "${TMP_PORTABLE_DIR}/Plugins" ]; then
    mkdir -p "${TMP_PORTABLE_DIR}/Plugins"
fi

# Copy Python distrib
# Remove it if it existed already (with DEBUG_SCRIPTS=1)
if [ -d "${TMP_PORTABLE_DIR}/lib/python${PYVER}" ]; then
    rm -rf "${TMP_PORTABLE_DIR}/lib/python${PYVER}"
fi

# The whitelist of python site-packages:
#python_site_packages=(easy_install.py easy_install.pyc pip pkg_resources PySide README setuptools wheel shiboken.so)
# Note that pip and dependencies were already installed by get-pip.py
# The PySide packages go to Plugins/ (which Natron adds to sys.path, see
# Global/PythonUtils.cpp), with a symbolic link in site-packages.
python_site_packages=(PySide2 shiboken2)
pyside_packages=(PySide2 shiboken2)

mkdir -p "${TMP_PORTABLE_DIR}/lib/python${PYVER}"

for pydir in "${SDK_HOME}/lib/python${PYVER}"; do
    (cd "$pydir"; tar cf - --exclude site-packages .)|(cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}"; tar xf -)
    for p in "${python_site_packages[@]}"; do
        if [ -e "$pydir/site-packages/$p" ]; then
            (cd "$pydir"; tar cf - "site-packages/$p") | (cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}"; tar xf -)
        fi
    done
done

# Move PySide to plug-ins directory and keep a symbolic link in site-packages
for p in "${pyside_packages[@]}"; do
    mv "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages/$p" "${TMP_PORTABLE_DIR}/Plugins/"
    (cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages"; ln -sf "../../../Plugins/$p" . )
done
# The PySide2 wheel's own Qt (PySide2/Qt) is not used: the SDK replaced its
# libraries with links to the SDK's Qt, which is bundled in lib/.
rm -rf "${TMP_PORTABLE_DIR}/Plugins/PySide2/Qt"

# Remove unused stuff
rm -rf "${TMP_PORTABLE_DIR}/lib/python${PYVER}"/{test,config,config-"${PYVER}${PYTHON_ABIFLAGS}"}
# The crypt module (deprecated since Python 3.11, removed in 3.13) needs libcrypt.so.2 (libxcrypt),
# which is not a system library everywhere: Debian and Ubuntu only have libcrypt.so.1.
rm -f "${TMP_PORTABLE_DIR}/lib/python${PYVER}/lib-dynload"/_crypt.*so

# Copy the SDK libraries needed by the Python extension modules (e.g. libreadline and
# libncursesw for readline and _curses)
PYDYNLOAD_DEPENDS=$(sdkDepends $(find "${TMP_PORTABLE_DIR}/lib/python${PYVER}/lib-dynload" -maxdepth 1 -type f -name '*.so'))
for y in $PYDYNLOAD_DEPENDS; do
    dep=$(basename "$y")
    if [ ! -f "${TMP_PORTABLE_DIR}/lib/$dep" ]; then
        cp -f "$y" "${TMP_PORTABLE_DIR}/lib/"
    fi
done

# Copy PySide dependencies
PYSIDE_DEPENDS=$(sdkDepends $(for p in "${pyside_packages[@]}"; do find "${SDK_HOME}/lib/python${PYVER}/site-packages/$p" -maxdepth 1 -type f -name '*.so*'; done))
for y in $PYSIDE_DEPENDS; do
    dep=$(basename "$y")
    if [ ! -f "${TMP_PORTABLE_DIR}/lib/$dep" ]; then
        cp -f "$y" "${TMP_PORTABLE_DIR}/lib/"
    fi
done

# Remove any pycache
(cd "${TMP_PORTABLE_DIR}" ; find . -name __pycache__ -exec rm -rf {} \;) || true

# Strip pyside and python depends
if [ "${COMPILE_TYPE}" != "debug" ]; then
    for p in "${pyside_packages[@]}"; do
        strip -s "${TMP_PORTABLE_DIR}/Plugins/$p"/* &>/dev/null || true
    done
    strip -s "${TMP_PORTABLE_DIR}/lib"/python*/* "${TMP_PORTABLE_DIR}/lib"/python*/*/* &>/dev/null || true
fi

# Remove pyo files and prune unneeded files using python-exclude file
PYDIR="${TMP_PORTABLE_DIR}/lib/python${PYVER:-}"
find "${PYDIR}" -type f -name '*.pyo' -exec rm {} \;
(cd "${PYDIR}"; xargs rm -rf || true) < "$INC_PATH/python-exclude.txt"

for p in "${pyside_packages[@]}"; do
    fixrpath "${TMP_PORTABLE_DIR}/Plugins/$p" "/../../lib"
done
fixrpath "${TMP_PORTABLE_DIR}/lib/python${PYVER:-}/lib-dynload ${TMP_PORTABLE_DIR}/lib/python${PYVER:-}/site-packages" "/../.."

export PY_BIN="${SDK_HOME}/bin/python${PYVER:-}"
export PYDIR="$PYDIR"
. "$CWD"/zip-python.sh

NATRON_PYTHON="${TMP_PORTABLE_DIR}/bin/natron-python"
# Install pip
if [ -x "${NATRON_PYTHON}" ]; then
    pushd "$PYDIR/../.."
    $CURL --remote-name --insecure https://bootstrap.pypa.io/get-pip.py
    "${NATRON_PYTHON}" get-pip.py
    rm get-pip.py
    # Install qtpy
    "${NATRON_PYTHON}" -m pip install qtpy
    # Useful Python packages
    "${NATRON_PYTHON}" -m pip install future six psutil
    # Run extra user provided pip install scripts
    if [ -f "${EXTRA_PYTHON_MODULES_SCRIPT:-}" ]; then
        "${NATRON_PYTHON}" "$EXTRA_PYTHON_MODULES_SCRIPT" || true
    fi
    "${NATRON_PYTHON}" -m compileall -f "${PYDIR}/site-packages"
    popd
fi


# Fix RPATH (we don't want to link against system libraries when deployed)
fixrpath "${TMP_PORTABLE_DIR}/bin" "/../lib"
fixrpath "${TMP_PORTABLE_DIR}/lib" ""
BIN_SUBDIRS=$(find "${TMP_PORTABLE_DIR}/bin" -type d ! -path "${TMP_PORTABLE_DIR}/bin")
fixrpath "$BIN_SUBDIRS" "/../../lib"


ALL_NATRON_LIBS=$(ls "${TMP_PORTABLE_DIR}/lib"/*.so*)

function installPlugin() {
    OFX_BINARY="$1"
    NATRON_LIBS="$2"

    if [ ! -d "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" ]; then
        return 0
    fi

    # plugins may keep references to /opt/Natron-sdk/lib64 (eg Arena.ofx or IO.ofx)
    fixrpath "${TMP_BINARIES_PATH}"/OFX/Plugins/"${OFX_BINARY}".ofx.bundle/Contents/Linux-* "/../../Libraries"

    # Copy to portable archive
    if [ ! -d "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron" ]; then
        mkdir -p "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"
    fi

    # Dump symbols
    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        "${SDK_HOME}/bin/dump_syms" "${TMP_BINARIES_PATH}"/OFX/Plugins/"${OFX_BINARY}".ofx.bundle/Contents/*/"${OFX_BINARY}".ofx > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${OFX_BINARY}.ofx-${CURRENT_DATE}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    fi




    # Extract dependencies
    OFX_DEPENDS="$(sdkDepends $(find "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle/Contents/Linux-"* -maxdepth 1 -type f))"
    if [ ! -z "$OFX_DEPENDS" ]; then


        LIBS_DIR="${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle/Libraries"
        mkdir -p "$LIBS_DIR"

        DEPENDS_NOT_PART_OF_NATRON=()
        for x in $OFX_DEPENDS; do
            # Add the dep only if it is not part of Natron libs
            pluginlib="$(basename "$x")"
            CREATE_SYM_LINK=0
            for y in $NATRON_LIBS; do
               natronlib="$(basename "$y")"
               if [ "$pluginlib" = "$natronlib" ]; then
                    CREATE_SYM_LINK=1
                    break
               fi
            done
            if [ "$CREATE_SYM_LINK" = "1" ]; then
                # Create a sym-link to the already bundled dependency in ${TMP_PORTABLE_DIR}/lib
                # This is a relative path, assuming the plug-in Libraries directory is:
                # ${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Libraries
                (cd "$LIBS_DIR"; ln -sf ../../../../../lib/"$pluginlib" .)
            else
                DEPENDS_NOT_PART_OF_NATRON+=("$x")
            fi
        done

        # Copy deps to Libraries directory in the plug-in bundle
        for x in "${DEPENDS_NOT_PART_OF_NATRON[@]-}"; do
            if [ ! -z "$x" ]; then
                pluginlib="$(basename "$x")"
                if [ -f "$x" ] && [ ! -f "$LIBS_DIR/$pluginlib" ] && [ ! -L "$LIBS_DIR/$pluginlib" ]; then
                    cp -f "$x" "$LIBS_DIR/"
                fi
            fi
        done

        # Extract dependencies of the dependencies
        OFX_LIB_DEP=$(sdkDepends $(find "$LIBS_DIR" -maxdepth 1 -type f))
        for y in $OFX_LIB_DEP; do
            pluginlib="$(basename "$y")"
            if [ ! -f "$LIBS_DIR/$pluginlib" ] && [ ! -L "$LIBS_DIR/$pluginlib" ]; then
                cp -f "$y" "$LIBS_DIR/"
            fi
        done

        # Set the rpath of the shared libraries to origin
        pushd "$LIBS_DIR"
        chmod 755 *.so* || true
        for i in *.so*; do
            patchelf --force-rpath --set-rpath "\$ORIGIN" "$i" || true
        done
        popd
    fi

    # Strip binary
    if [ "$COMPILE_TYPE" != "debug" ]; then
        find "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" -type f \( -iname '*.so' -o -iname '*.ofx' \) -exec strip -s {} \; &>/dev/null
    fi

    # Copy plug-in bundle
    cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/"

}

installPlugin "IO" "$ALL_NATRON_LIBS"
installPlugin "Misc" "$ALL_NATRON_LIBS"
installPlugin "CImg" "$ALL_NATRON_LIBS"
installPlugin "Shadertoy" "$ALL_NATRON_LIBS"
installPlugin "Arena" "$ALL_NATRON_LIBS"
installPlugin "ArenaCL" "$ALL_NATRON_LIBS"
installPlugin "GMIC" "$ALL_NATRON_LIBS"

# Generate documentation
bash "$CWD"/gen-natron-doc.sh

# No system runtime may be bundled, and the binaries must run with the system libraries of
# the oldest supported distribution (Rocky Linux 9)
checkBundle "${TMP_PORTABLE_DIR}"

# The OFX plug-in bundles, with openfx's linuxCheckBundle.sh: it also checks their DT_RUNPATH/
# DT_RPATH, which fixrpath and installPlugin set with "patchelf ... || true", and that every
# library they need is found through them. It is run on the portable tree, where the symlinks
# from the plug-ins' Libraries directories to Natron's lib directory resolve. The desktop
# libraries of SYSTEM_LIBS_RE are system libraries for it too.
OFX_CHECK_BUNDLE="$TMP_PATH/Natron/libs/OpenFX/Support/include/linuxCheckBundle.sh"
if [ -f "$OFX_CHECK_BUNDLE" ]; then
    env OFX_ABI_SYSTEM_LIBS="$(systemLibsNeeded "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron" | tr '\n' ' ')" bash "$OFX_CHECK_BUNDLE" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"
else
    echo "Warning: $OFX_CHECK_BUNDLE not found (libs/OpenFX predates openfx#159): the rpaths of the OFX plug-in bundles are not checked"
fi

# At this point we can run Natron unit tests to check that the deployment is ok.
rm -rf "$HOME/.cache/INRIA/Natron"* &> /dev/null || true
# The tests need no display: use Qt's offscreen platform with or without valgrind.
if command -v valgrind > /dev/null 2>&1; then
    env QT_QPA_PLATFORM=offscreen $TIMEOUT -s KILL 1800 valgrind --tool=memcheck --suppressions="$INC_PATH/natron/valgrind-python${PYV}.supp" "${TMP_PORTABLE_DIR}/bin/Tests"
else
    # If valgrind is not installed in the build environment, run the tests without it.
    echo "Warning: valgrind not found, running the bundled unit tests without it"
    env QT_QPA_PLATFORM=offscreen $TIMEOUT -s KILL 1800 "${TMP_PORTABLE_DIR}/bin/Tests"
fi
rm "${TMP_PORTABLE_DIR}/bin/Tests"

if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi

mkdir -p "${BUILD_ARCHIVE_DIRECTORY}"

if [ "$DISABLE_PORTABLE_ARCHIVE" != "1" ]; then
    # Portable archive
    (cd "${TMP_BINARIES_PATH}" && tar Jcf "${PORTABLE_DIRNAME}.tar.xz" "${PORTABLE_DIRNAME}"; mv "${PORTABLE_DIRNAME}.tar.xz" "${BUILD_ARCHIVE_DIRECTORY}/${PORTABLE_DIRNAME}.tar.xz")
fi

echo "*** Artifacts:"
ls -R  "${BUILD_ARCHIVE_DIRECTORY}"
echo "*** Linux installer: done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
