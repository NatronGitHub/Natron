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
