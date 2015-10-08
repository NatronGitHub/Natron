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

export LD_LIBRARY_PATH="$DIR/lib:$DIR/Plugins/IO.ofx.bundle/Libraries"
"$DIR/bin/ffmpeg" "$@"
