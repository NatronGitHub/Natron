#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x # Print commands and their arguments as they are executed.

# Options:
# PY_BIN: Python binary path
# PYDIR: Path to python lib path

echo "* Zipping Python..."

pushd "$PYDIR"
# remove pyc files
find . -name '*.pyc' -print0 | xargs rm
find . -name '*.py' -print0 | xargs chmod 644
# generate pyo files
"$PY_BIN" -O -m compileall -q . || true
PY_ZIP_FILES=(*.py* bsddb compiler ctypes curses distutils email encodings hotshot idlelib importlib json logging multiprocessing pydoc_data sqlite3 unittest wsgiref xml)

echo "** env:"
env
echo "** zip: $(type zip)"
# python zip
echo "** zipping" "${PY_ZIP_FILES[@]}"
if [ "$PKGOS" = "Windows" ]; then
    zip -qr "../python${PYVERNODOT}.zip" "${PY_ZIP_FILES[@]}"
else
    env -i zip -qr "../python${PYVERNODOT}.zip" "${PY_ZIP_FILES[@]}"
fi
#echo "** testing zipped file"
#unzip -tq ../python${PYVERNODOT}.zip
#unzip -lv ../python${PYVERNODOT}.zip
rm -rf "${PY_ZIP_FILES[@]}" || true

# echo "** second generation zip"
# # build a second-generation zip
# cd ..
# mkdir tmp
# cd tmp
# unzip ../python${PYVERNODOT}.zip
# xattr -dr com.apple.quarantine .
# mv ../python${PYVERNODOT}.zip ../python${PYVERNODOT}.zip.0
# echo "** zipping $PY_ZIP_FILES"
# env -i zip -v -r ../python${PYVERNODOT}.zip $PY_ZIP_FILES
# echo "** testing zipped file"
# ls -la ../python${PYVERNODOT}.zip
# unzip -tq ../python${PYVERNODOT}.zip
# unzip -lv ../python${PYVERNODOT}.zip
# cd ..
# rm -rf tmp

popd # pushd "$PYDIR"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
