#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.
#set -v # Prints shell input lines as they are read.

if [ $# != 1 ]; then
    echo "Usage: $0 /path/to/Application.app/Contents/MacOS/executable_to_fix"
    echo "Move MacPorts dependencies of an app binary to Application.app/Contents/Frameworks"
    exit 1
fi
exe=$1
pkglib="$(dirname $exe)/../Frameworks"
SDK_HOME="${SDK_HOME:-/opt/local}"

MPLIBS0="$(otool -L "$exe" | grep -F "${SDK_HOME}/lib" | grep -F -v ':' |sort|uniq |awk '{print $1}')"
# also add first-level and second-level dependencies 
MPLIBS1="$(for i in $MPLIBS0; do echo "$i"; otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
MPLIBS="$(for i in $MPLIBS1; do echo "$i"; otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
for mplib in $MPLIBS; do
    if [ ! -f "$mplib" ]; then
        echo "missing $exe depend $mplib"
        exit 1
    fi
    lib="$(echo "$mplib" | awk -F / '{print $NF}')"
    if [ ! -f "$pkglib/${lib}" ]; then
        echo "copying missing lib ${lib}"
        cp "$mplib" "$pkglib/${lib}"
        chmod +w "$pkglib/${lib}"
    fi
    install_name_tool -change "$mplib" "@executable_path/../Frameworks/$lib" "$exe"
done
