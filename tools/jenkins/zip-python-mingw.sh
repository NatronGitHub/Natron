#!/bin/bash

# Create Python3 embed zip for msys2/mingw

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.

PYTEMP=${PYTMP:-`pwd`/python-embed}
PYVER=${PYVER:-3.10}
PYVERNODOT=${PYVERNODOT:-`echo ${PYVER} | sed -e 's/\.//g'`}

echo "* Creating Python ${PYVER} (${PYVERNODOT}) embed zip in ${PYTEMP} ..."

PY_PKG=`pacman -Q mingw-w64-x86_64-python | awk '{print $2}'`
PY_PKG_FILE="mingw-w64-x86_64-python-${PY_PKG}-any.pkg.tar.zst"
PY_PKG_URL="https://repo.msys2.org/mingw/x86_64/${PY_PKG_FILE}"

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

mkdir -p "${PYTEMP}" || true
rm -rf "${EMBED}" || true
rm -f "${EMBED}.zip" || true
mkdir -p "${EMBED}"

cd "${PYTEMP}"

if [ ! -f "${PY_PKG_FILE}" ]; then
    wget ${PY_PKG_URL}
fi
if [ ! -d "${SRC}" ]; then
    tar xf "${PY_PKG_FILE}"
fi

for PYDIR in ${PYDIRS}; do
    cp -a "${SRC}/${PYDIR}" "${EMBED}/"
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

(cd ${EMBED} ;  zip -9 -r ../python${PYVERNODOT}.zip .)

echo "==> ${EMBED}.zip is done!"
