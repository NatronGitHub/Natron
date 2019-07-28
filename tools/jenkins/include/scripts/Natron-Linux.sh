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

# Check gcc compat
compat_arch="$(uname -m)"
compat_version=3.4.19 # modified by build-Linux-installer.sh
if [ "$compat_arch" = "x86_64" ]; then
    compat_suffix=64
fi
if [ -f "/usr/lib${compat_suffix}/libstdc++.so.6" ]; then
    stdc_lib="/usr/lib${compat_suffix}/libstdc++.so.6"
elif [ -f "/usr/lib/libstdc++.so.6" ]; then
    stdc_lib="/usr/lib/libstdc++.so.6"
elif [ -f "/usr/lib/${compat_arch}-linux-gnu/libstdc++.so.6" ]; then
    stdc_lib="/usr/lib/${compat_arch}-linux-gnu/libstdc++.so.6"
elif [ -f "/usr/lib/i386-linux-gnu/libstdc++.so.6" ]; then
    stdc_lib="/usr/lib/i386-linux-gnu/libstdc++.so.6"
fi
# if [ -n "${stdc_lib+x}" ]; then
#     compat_gcc="$("$dir/bin/strings" "$stdc_lib" | grep GLIBCXX_${compat_version})"
# fi
## Former version, which can only be executed by root... !!!
# if [ -n "${compat_gcc+x}" ]; then
#     if [ -f "$dir/lib/libstdc++.so.6" ]; then
#         rm -f "$dir/lib/libstdc++.so.6" || echo "Failed to remove symlink, please run as root to fix."
#     fi
#     if [ -f "$dir/lib/libgcc_s.so.1" ]; then
#         rm -f "$dir/lib/libgcc_s.so.1" || echo "Failed to remove symlink, please run as root to fix."
#     fi
#     if [ -f "$dir/lib/libgomp.so.1" ]; then
#         rm -f "$dir/lib/libgomp.so.1" || echo "Failed to remove symlink, please run as root to fix."
#     fi
# else
#     if [ ! -f "$dir/lib/libstdc++.so.6" ]; then
#         ln -sf compat/libstdc++.so.6 "$dir/lib" || echo "Failed to create symlink, please run as root to fix."
#     fi
#     if [ ! -f "$dir/lib/libgcc_s.so.1" ]; then
#         ln -sf compat/libgcc_s.so.1 "$dir/lib" || echo "Failed to create symlink, please run as root to fix."
#     fi
#     if [ ! -f "$dir/lib/libgomp.so.1" ]; then
#         ln -sf compat/libgomp.so.1 "$dir/lib" || echo "Failed to create symlink, please run as root to fix."
#     fi
# fi

# if [ -z "${compat_gcc+x}" ]; then
#     # never set LD_LIBRARY_PATH, because it breaks third-party plugins!
#     # normally, patchelf was used on every binary, so that the correct libraries should be found
#     #export LD_LIBRARY_PATH="$dir/lib/compat${LD_LIBRARY_PATH+:}${LD_LIBRARY_PATH:-}"

#     # We don't care: always use the provided libraries
#     true
# fi

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
