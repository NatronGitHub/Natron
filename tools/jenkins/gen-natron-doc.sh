#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x # Print commands and their arguments as they are executed.

source common.sh
source manageBuildOptions.sh
source manageLog.sh
updateBuildOptions

printStatusMessage "Building documentation..."

NATRON_DOC_PATH="${TMP_PATH}/Natron/Documentation"
pushd "$NATRON_DOC_PATH"

# generating the docs requires a recent pandoc, else a spurious "{ width=10% }" will appear after each plugin logo
# pandoc 1.19.1 runs OK, but MacPorts has an outdated version (1.12.4.2), and Ubuntu 16LTS has 1.16.0.2
# For now, we regenerate the docs from a CI or SNAPSHOT build on Fred's mac, using:
# cd Development/Natron/Documentation
# env FONTCONFIG_FILE=/Applications/Natron.app/Contents/Resources/etc/fonts/fonts.conf OCIO=/Applications/Natron.app/Contents/Resources/OpenColorIO-Configs/nuke-default/config.ocio OFX_PLUGIN_PATH=/Applications/Natron.app/Contents/Plugins bash ../tools/genStaticDocs.sh /Applications/Natron.app/Contents/MacOS/NatronRenderer  $TMPDIR/natrondocs .
GENDOCS=1
if [ "$PKGOS" = "OSX" ]; then
    RES_DIR="${TMP_PORTABLE_DIR}.app/Contents/Resources"
else
    RES_DIR="$TMP_PORTABLE_DIR/Resources"
fi
if [ "$GENDOCS" = 1 ] && command -v pandoc >/dev/null 2>&1; then

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    NATRON_RENDERER_BIN="NatronRenderer-bin"
else
    NATRON_RENDERER_BIN="NatronRenderer"
fi

# generate the plugins doc first
SPHINX_BIN=""
if [ "$PKGOS" = "Linux" ]; then
    SPHINX_BIN="sphinx-build"
    export LD_LIBRARY_PATH="${SDK_HOME}/lib" # python3 depends on libs in SDK_HOME/lib
    #"$NATRON_DOC_PATH"/../tools/genStaticDocs.sh "$TMP_PORTABLE_DIR/bin/$NATRON_RENDERER_BIN" "$TMPDIR/natrondocs$$" .
elif [ "$PKGOS" = "Windows" ]; then
    #SPHINX_BIN="sphinx-build2.exe" # fails with 'failed to create process (\mingw64\bin\python2.exe "C:\msys64\mingw64\bin\sphinx-build2-script.py").'
    SPHINX_BIN="sphinx-build2-script.py"
    #"$NATRON_DOC_PATH"/../tools/genStaticDocs.sh "$TMP_PORTABLE_DIR/bin/${NATRON_RENDERER_BIN}.exe" "$TMPDIR/natrondocs$$" .
elif [ "$PKGOS" = "OSX" ]; then
    RES_DIR="$TMP_PORTABLE_DIR/Contents/Resources"
    SPHINX_BIN="sphinx-build-${PYVER}"
    #"$NATRON_DOC_PATH"/../tools/genStaticDocs.sh "${TMP_PORTABLE_DIR}.app/Contents/MacOS/$NATRON_RENDERER_BIN" "$TMPDIR/natrondocs$$" .
fi
$SPHINX_BIN -b html source html
if [ ! -d "$RES_DIR/docs" ]; then
    mkdir -p "$RES_DIR/docs"
fi
cp -a html "$RES_DIR/docs/"
fi


GENPDF=0
if [ "$GENPDF" = 1 ] && [ ! -f "$RES_DIR/docs/Natron.pdf" ]; then
    $SPHINX_BIN -b latex source pdf
    pushd pdf
    # we have to run twice pdflatex, then makeindex, then pdflatex again
    pdflatex -shell-escape -interaction=nonstopmode -file-line-error Natron.tex | grep ".*:[0-9]*:.*"
    pdflatex -shell-escape -interaction=nonstopmode -file-line-error Natron.tex | grep ".*:[0-9]*:.*"
    makeindex Natron
    pdflatex -shell-escape -interaction=nonstopmode -file-line-error Natron.tex | grep ".*:[0-9]*:.*"
    popd
    cp pdf/Natron.pdf "$RES_DIR/docs/"
fi

popd

echo "Done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
