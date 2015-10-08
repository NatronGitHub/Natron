#!/bin/bash
#
# Universal launch script for Natron
#

# Get real current dir
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
    SOURCEDIR=`dirname "$SOURCE"`
    DIR=`cd -P "$SOURCEDIR" && pwd`
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SOURCEDIR=`dirname "$SOURCE"`
DIR=`cd -P "$SOURCEDIR" && pwd`

# Force numeric
export LC_NUMERIC=C

# Set fontconfig path
# Not needed (done in app), but added to avoid warn before splashscreen
if [ -d "$DIR/Resources/etc/fonts" ]; then
  export FONTCONFIG_PATH="$DIR/Resources/etc/fonts"
fi

# Check gcc compat
COMPAT_ARCH=`uname -m`
COMPAT_VERSION=3.4.15
if [ "$COMPAT_ARCH" = "x86_64" ]; then
  COMPAT_SUFFIX=64
fi
if [ -f /usr/lib$COMPAT_SUFFIX/libstdc++.so.6 ]; then
  STDC_LIB=/usr/lib$COMPAT_SUFFIX/libstdc++.so.6
elif [ -f /usr/lib/libstdc++.so.6 ]; then
  STDC_LIB=/usr/lib/libstdc++.so.6
elif [ -f /usr/lib/$COMPAT_ARCH-linux-gnu/libstdc++.so.6 ]; then
  STDC_LIB=/usr/lib/$COMPAT_ARCH-linux-gnu/libstdc++.so.6
elif [ -f /usr/lib/i386-linux-gnu/libstdc++.so.6 ]; then
  STDC_LIB=/usr/lib/i386-linux-gnu/libstdc++.so.6
fi
if [ "$STDC_LIB" != "" ]; then
  COMPAT_GCC=`$DIR/bin/strings $STDC_LIB | grep GLIBCXX_${COMPAT_VERSION}`
fi
if [ "$COMPAT_GCC" = "GLIBCXX_${COMPAT_VERSION}" ]; then
  if [ -f "$DIR/lib/libstdc++.so.6" ]; then
    rm -f $DIR/lib/libstdc++.so.6 || echo "Failed to remove symlink, please run as root to fix."
  fi
  if [ -f "$DIR/lib/libgcc_s.so.1" ]; then
    rm -f $DIR/lib/libgcc_s.so.1 || echo "Failed to remove symlink, please run as root to fix."
  fi
  if [ -f "$DIR/lib/libgomp.so.1" ]; then
    rm -f $DIR/lib/libgomp.so.1 || echo "Failed to remove symlink, please run as root to fix."
  fi
else
  if [ ! -f "$DIR/lib/libstdc++.so.6" ]; then
    cd $DIR/lib ; ln -sf compat/libstdc++.so.6 . || echo "Failed to create symlink, please run as root to fix."
  fi
  if [ ! -f "$DIR/lib/libgcc_s.so.1" ]; then
    cd $DIR/lib ; ln -sf compat/libgcc_s.so.1 . || echo "Failed to create symlink, please run as root to fix."
  fi
  if [ ! -f "$DIR/lib/libgomp.so.1" ]; then
    cd $DIR/lib ; ln -sf compat/libgomp.so.1 . || echo "Failed to create symlink, please run as root to fix."
  fi
fi

# Check for updates
if [ "$1" = "-update" -a -x "$DIR/NatronSetup" ]; then
    "$DIR/NatronSetup" --updater
    exit 0
fi

# Portable mode, save settings in current dir
if [ "$1" = "-portable" ]; then
    #XDG_CACHE_HOME=/tmp
    XDG_DATA_HOME="$DIR"
    XDG_CONFIG_HOME="$DIR"
    export XDG_DATA_HOME XDG_CONFIG_HOME
fi

# start app, with optional debug
if [ "$1" = "-debug" -a -x "$DIR/bin/Natron.debug" ]; then
    SEGFAULT_SIGNALS="all"
    export SEGFAULT_SIGNALS
    catchsegv "$DIR/bin/Natron.debug" -style fusion "$@"
else
    "$DIR/bin/Natron" -style fusion "$@"
fi
