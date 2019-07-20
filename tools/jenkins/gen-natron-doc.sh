#!/bin/bash

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

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
GENDOCS=true
RUNPANDOC=false
GENPDF=false

if [ "$PKGOS" = "OSX" ]; then
    RES_DIR="${TMP_PORTABLE_DIR}.app/Contents/Resources"
else
    RES_DIR="$TMP_PORTABLE_DIR/Resources"
fi
[ ! -d "$RES_DIR/docs" ] && mkdir -p "$RES_DIR/docs"

if "$GENDOCS"; then

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        NATRON_RENDERER_BIN="NatronRenderer-bin"
    else
        NATRON_RENDERER_BIN="NatronRenderer"
    fi

    # generate the plugins doc first
    if "$RUNPANDOC" && command -v pandoc >/dev/null 2>&1 && [ $(pandoc --version |head -1 |awk '{print $2}') == "2.3.1" ]; then
        if [ "$PKGOS" = "Linux" ]; then
            env LD_LIBRARY_PATH="${SDK_HOME}/lib" "$NATRON_DOC_PATH"/../tools/genStaticDocs.sh "$TMP_PORTABLE_DIR/bin/$NATRON_RENDERER_BIN" "$TMPDIR/natrondocs$$" .
        elif [ "$PKGOS" = "Windows" ]; then
            "$NATRON_DOC_PATH"/../tools/genStaticDocs.sh "$TMP_PORTABLE_DIR/bin/${NATRON_RENDERER_BIN}.exe" "$TMPDIR/natrondocs$$" .
        elif [ "$PKGOS" = "OSX" ]; then
            "$NATRON_DOC_PATH"/../tools/genStaticDocs.sh "${TMP_PORTABLE_DIR}.app/Contents/MacOS/$NATRON_RENDERER_BIN" "$TMPDIR/natrondocs$$" .
        fi
    fi

    if [ "$PKGOS" = "Linux" ]; then
        SPHINX_BIN="sphinx-build"
    elif [ "$PKGOS" = "Windows" ]; then
        #SPHINX_BIN="sphinx-build2.exe" # fails with 'failed to create process (\mingw64\bin\python2.exe "C:\msys64\mingw64\bin\sphinx-build2-script.py").'
        SPHINX_BIN="sphinx-build2-script.py"
    elif [ "$PKGOS" = "OSX" ]; then
        SPHINX_BIN="sphinx-build-${PYVER}"
    fi
    $SPHINX_BIN -b html source html
    cp -R html "$RES_DIR/docs/"
fi


if "$GENPDF" && command -v pdflatex >/dev/null 2>&1 && [ ! -f "$RES_DIR/docs/Natron.pdf" ]; then
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
