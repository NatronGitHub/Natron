#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

# Options:
# PY_BIN: Python binary path
# PYDIR: Path to python lib path

# Python 3: the standard library comes from the embedded Python zip
if [ "$PYV" = 3 ]; then
    echo "* Downloading Embedded Python zip..."
    pushd "$PYDIR"
    mv site-packages lib-dynload config-${PYVER}-* _sysconfigdata*.py* ..
    cd ..
    PYVERFULL="$(python3 -c "import platform; print('.'.join(platform.python_version_tuple()))")"
    if [ "$PYVER" = 3.9 ] && version_gt "$PYVERFULL" 3.9.13; then
        # 3.9.14 and later don't have embedded zips
        PYVERFULL=3.9.13
    fi
    if [ "$PYVER" = 3.10 ] && version_gt "$PYVERFULL" 3.10.11; then
        # 3.10.12 and later don't have embedded zips
        PYVERFULL=3.10.11
    fi
    if [ "$PYVER" = 3.11 ] && version_gt "$PYVERFULL" 3.11.9; then
        # 3.11.10 and later (security fixes only) don't have embedded zips.
        # The bytecode format is the same across 3.11.x, but the bundled stdlib then misses the
        # fixes of the later releases. TODO: build python${PYVERNODOT}.zip from the SDK's own
        # stdlib (compileall + zip) instead of downloading it.
        PYVERFULL=3.11.9
    fi
    # curl (unlike wget) honors ALL_PROXY, e.g. a SOCKS proxy in a build VM; -f: fail on HTTP errors
    curl -f -L -sS --retry 5 --connect-timeout 30 -O "https://www.python.org/ftp/python/${PYVERFULL}/python-${PYVERFULL}-embed-amd64.zip"
    unzip "python-${PYVERFULL}-embed-amd64.zip" "python${PYVERNODOT}.zip"
    rm "python-${PYVERFULL}-embed-amd64.zip"
    rm -rf python${PYVER}
    mkdir python${PYVER}
    mv site-packages lib-dynload config-${PYVER}-* _sysconfigdata*.py* python${PYVER}
    popd
fi

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
