#!/bin/bash

# Create Python3 embed zip for msys2/mingw

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x

# PYDIR: Path to python lib path
PYTEMP=${PYTEMP:-${PYDIR}/zip_tmp}
PYVER=${PYVER:-3.10}
PYVERNODOT=${PYVERNODOT:-`echo ${PYVER} | sed -e 's/\.//g'`}

PY_PKG=`pacman -Q mingw-w64-x86_64-python | awk '{print $2}'`
PY_PKG_FILE="mingw-w64-x86_64-python-${PY_PKG}-any.pkg.tar.zst"
PY_PKG_URL="https://repo.msys2.org/mingw/x86_64/${PY_PKG_FILE}"
PACMAN_CACHE="/var/cache/pacman/pkg"

PYDIRS="
asyncio
collections
concurrent
ctypes
curses
dbm
distutils
email
encodings
html
http
importlib
json
lib2to3
logging
msilib
multiprocessing
pydoc_data
re
sqlite3
unittest
urllib
wsgiref
xml
xmlrpc
zoneinfo
"

EMBED="${PYTEMP}/python${PYVERNODOT}"
SRC="${PYTEMP}/mingw64/lib/python${PYVER}"

mkdir -p "${EMBED}"

if [ ! -d "${PACMAN_CACHE}" ]; then
    mkdir -p "${PACMAN_CACHE}"
fi
if [ ! -f "${PACMAN_CACHE}/${PY_PKG_FILE}" ]; then
    wget ${PY_PKG_URL} -O "${PACMAN_CACHE}/${PY_PKG_FILE}"
fi
if [ ! -d "${SRC}" ]; then
    tar xf "${PACMAN_CACHE}/${PY_PKG_FILE}" -C "${PYTEMP}"
fi

for dir in ${PYDIRS}; do
    cp -a "${SRC}/${dir}" "${EMBED}/"
done

mkdir -p "${EMBED}/site-packages"
cp -a "${SRC}/"*.py "${EMBED}/"

(cd ${EMBED} ; find . -name "__pycache__" -type d -exec rm -r {} +)
(cd ${EMBED} ; find . -name "test" -type d -exec rm -r {} +)
(cd ${EMBED} ; find . -name "tests" -type d -exec rm -r {} +)

python${PYVER} -m compileall -b -f ${EMBED}

(cd ${EMBED} ; find . -name "__pycache__" -type d -exec rm -r {} +)
(cd ${EMBED} ; find . -name "*.py" -type f -delete)
(cd ${EMBED} ; find . -name "*.exe" -type f -delete)
(cd ${EMBED} ; find . -name "*.bat" -type f -delete)
(cd ${EMBED} ; find . -name "README.*" -type f -delete)

(cd ${EMBED} ;  zip -9 -r ${PYDIR}/../python${PYVERNODOT}.zip .)

(cd "$PYDIR" ;
mv site-packages lib-dynload _sysconfigdata*.py* ..
cd ..
rm -rf python${PYVER}
mkdir python${PYVER}
mv site-packages lib-dynload _sysconfigdata*.py* python${PYVER}/
)

rm -f ${PYDIR}/site-packages/sphinxcontrib* || true
rm -f ${PYDIR}/site-packages/*.dll.a || true
