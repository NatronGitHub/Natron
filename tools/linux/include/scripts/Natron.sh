#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
    SOURCEDIR=`dirname "$SOURCE"`
    DIR=`cd -P "$SOURCEDIR" && pwd`
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SOURCEDIR=`dirname "$SOURCE"`
DIR=`cd -P "$SOURCEDIR" && pwd`

export LC_NUMERIC=C

if [ "$1" = "-update" -a -x "$DIR/NatronSetup" ]; then
    "$DIR/NatronSetup" --updater
    exit 0
fi

if [ "$1" = "-portable" ]; then
    #XDG_CACHE_HOME=/tmp
    XDG_DATA_HOME=$DIR
    XDG_CONFIG_HOME=$DIR
    export XDG_DATA_HOME XDG_CONFIG_HOME
fi

if [ "$1" = "-debug" ]; then
    SEGFAULT_SIGNALS="all"
    export SEGFAULT_SIGNALS
    catchsegv "$DIR/bin/Natron.debug" -style fusion "$@"
else
    "$DIR/bin/Natron" -style fusion "$@"
fi
