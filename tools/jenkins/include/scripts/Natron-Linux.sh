#!/bin/bash
#
# Universal launch script for Natron
#

#set -e # Exit immediately if a command exits with a non-zero status
#set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

# parse command-line options
help=0
update=0
portable=0
debug=0
nocrashdump=0
parseoptions=1
while [ "$parseoptions" = 1 ]; do
    case $1 in
        -u|--update)
            update=1
            shift # past argument
            ;;
        -p|--portable)
            portable=1
            shift # past argument
            ;;
        -d|--debug)
            debug=1
            shift # past argument
            ;;
        -n|--nocrashdump)
            nocrashdump=1
            shift # past argument
            ;;
        -h|--help)
            help=1
            # leave the argument here and stop option parsing
            parseoptions=0
            ;;
        *)
            # unknown option
            parseoptions=0
            ;;
    esac
done

# Get real current dir
source="${BASH_SOURCE[0]}"
while [ -h "$source" ]; do
    sourcedir="$(dirname "$source")"
    dir="$(cd -P "$sourcedir" && pwd)"
    source="$(readlink "$source")"
    [[ "$source" != /* ]] && source="$dir/$source"
done
sourcedir="$(dirname "$source")"
dir="$(cd -P "$sourcedir" && pwd)"

# Force numeric
export LC_NUMERIC=C

# glibc write to stderr
export LIBC_FATAL_STDERR_=1

# KDE4 workaround (QTBUG-30981)
export QT_PLUGIN_PATH=""

# Set fontconfig path
# Not needed (done in app), but added to avoid warn before splashscreen
if [ -d "$dir/Resources/etc/fonts" ]; then
    export FONTCONFIG_PATH="$dir/Resources/etc/fonts"
fi

BUNDLE_LIBRARY_PATH=${dir}/lib
for lib in "$dir/Plugins/OFX/Natron/"*.ofx.bundle; do
    if [ -d "${lib}/Libraries" ]; then
        BUNDLE_LIBRARY_PATH="${BUNDLE_LIBRARY_PATH}:${lib}/Libraries"
    fi
done
# never set LD_LIBRARY_PATH, because it breaks third-party plugins!
# normally, patchelf was used on every binary, so that the correct libraries should be found
#export LD_LIBRARY_PATH="$BUNDLE_LIBRARY_PATH${LD_LIBRARY_PATH+:}${LD_LIBRARY_PATH:-}"

# libc, libstdc++, libgcc_s, libgomp, OpenGL and X11 are never bundled: they always come
# from the system (see SYSTEM_LIBS_RE in build-Linux-installer.sh)

# Check for updates
if [ "$update" = 1 ] && [ -x "$dir/NatronSetup" ]; then
    exec "$dir/NatronSetup" --updater
fi

# Portable mode, save settings in current dir
if [ "$portable" = 1 ]; then
    #export XDG_CACHE_HOME=/tmp
    export XDG_DATA_HOME="$dir"
    export XDG_CONFIG_HOME="$dir"
fi

if [ "$help" = 1 ]; then
    echo "Usage: $1 [-u|--update] [-p|--portable] [-n|--nocrashdump] [-d|--debug] <options>"
    echo "Full usage:"
fi        

bin="$dir/bin/Natron"

if [ "$nocrashdump" = 1 ] && [ -x "${bin}-bin" ]; then
    bin="${bin}-bin"
fi        

# start app, with optional debug
if [ "$debug" = 1 ] && [ -x "${bin}.debug" ]; then
    export SEGFAULT_SIGNALS="all"
    exec catchsegv "${bin}.debug" "$@"
else
    exec "$bin" "$@"
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
