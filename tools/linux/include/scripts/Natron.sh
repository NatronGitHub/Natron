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
    #export XDG_CACHE_HOME=/tmp
    export XDG_DATA_HOME=$DIR
    export XDG_CONFIG_HOME=$DIR
fi

if [ "$1" = "-debug" ]; then
    export SEGFAULT_SIGNALS="all"
    catchsegv "$DIR/bin/Natron.debug" -style fusion "$@"
else
    "$DIR/bin/Natron" -style fusion "$@"
fi
