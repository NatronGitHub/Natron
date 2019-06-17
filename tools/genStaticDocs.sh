#!/bin/bash
# regenerate static documentation.
# Should be used using a snapshot build, eg:
# env FONTCONFIG_FILE=/Applications/Natron.app/Contents/Resources/etc/fonts/fonts.conf OCIO=/Applications/Natron.app/Contents/Resources/OpenColorIO-Configs/nuke-default/config.ocio OFX_PLUGIN_PATH=/Applications/Natron.app/Contents/Plugins ./genStaticDocs.sh ~/Development/Natron-2.1/Renderer/build/Debug/NatronRenderer  /var/tmp/natrondocs ~/Development/Natron-2.1/Documentation

#set -v
#set -x
set -u
set -e

NATRON_BIN="${1:-}"
TMP_FOLDER="${2:-}"
DOC_FOLDER="${3:-}"

if [ -z "$NATRON_BIN" ] || [ -z "$TMP_FOLDER" ] || [ -z "$DOC_FOLDER" ]; then
    echo "Usage: script NATRON_RENDERER TMP_FOLDER DOC_FOLDER"
    exit 1
fi

#PANDOC="$HOME/.cabal/bin/pandoc"
PANDOC=pandoc
# Note than in pandoc >= 2.4 there is no way to force grid tables in rst output.
# See https://github.com/jgm/pandoc/issues/5393
pandocversion=$($PANDOC --version |head -1 | awk '{print $2}')
# see https://stackoverflow.com/a/24067243
function version_gt() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }

if version_gt "$pandocversion" "2.3.1"; then
    echo "Error: pandoc >= 2.4 cannot be forced to produce grid tables in rst output"
    echo "Maybe it was fixed in your version (pandoc $pandocversion)? Please check https://github.com/jgm/pandoc/issues/5393"
    echo "Fix $0 if you think your version works to produce grid tables,"
    echo "or modify the 'multiline table element hack' in convertFromPlainTextToMarkdown() and Node::makeDocumentation()"
    echo "In the meantime, pandoc 2.3.1 binaries can be downloaded from https://github.com/jgm/pandoc/releases/2.3.1"
    exit
fi

SED=sed
if [ "$(uname -s)" = "Darwin" ]; then
    SED=gsed
fi

if [ -d "$DOC_FOLDER/source" ]; then
    rm -rf "$DOC_FOLDER/source/_group*" "$DOC_FOLDER/source/_prefs.rst" "$DOC_FOLDER/source/plugins" || true
fi

if [ ! -d "$TMP_FOLDER" ]; then
    mkdir -p "$TMP_FOLDER" || exit 1
fi
rm -rf "${TMP_FOLDER:?}"/* || true
if [ ! -d "$TMP_FOLDER/plugins" ]; then
    mkdir -p "$TMP_FOLDER/plugins" || exit 1
fi
if [ ! -d "$DOC_FOLDER" ]; then
    mkdir -p "$DOC_FOLDER" || exit 1
fi
if [ ! -d "$DOC_FOLDER/source/plugins" ]; then
    mkdir -p "$DOC_FOLDER/source/plugins" || exit 1
fi

OPTS=("--no-settings")
if [ -n "${OFX_PLUGIN_PATH:-}" ]; then
    echo "OFX_PLUGIN_PATH=${OFX_PLUGIN_PATH:-}, setting useStdOFXPluginsLocation=False"
    OPTS=(${OPTS[@]+"${OPTS[@]}"} "--setting" "useStdOFXPluginsLocation=False")
fi

touch "$TMP_FOLDER/dummy.ntp"
"$NATRON_BIN" ${OPTS[@]+"${OPTS[@]}"} --export-docs "$TMP_FOLDER" "$TMP_FOLDER/dummy.ntp"
rm "$TMP_FOLDER/dummy.ntp"

pushd "$TMP_FOLDER"

for i in *.md; do
    $PANDOC -f markdown -t rst-simple_tables-multiline_tables+grid_tables "$i" --columns=1000 -o "$(echo "$i"|$SED 's/\.md$/.rst/')"
#    $PANDOC "$i" --columns=1000 -o "$(echo "$i"|$SED 's/\.md$/.rst/')"
done
for x in plugins/*.md; do
    $PANDOC "$x" --columns=9000 -o "$(echo "$x"|$SED 's/\.md$/.rst/')"
    PLUG="$(echo "$x"|$SED 's/.md//;s#plugins/##')"
    $SED -i "1i.. _${PLUG}:\n" plugins/"${PLUG}".rst
    # multiline table element hack: in tables (lines beginning
    # with '|') replace "| . " with "| | "
    # see comments in source code in functions:
    # - convertFromPlainTextToMarkdown()
    # - Node::makeDocumentation()
    $SED -i '/^| /s/| \. /| | /g' plugins/"${PLUG}".rst
    # add properties for the Controls tables
    $SED -i 's/^CONTROLSTABLEPROPS/.. tabularcolumns:: |>{\\raggedright}p{0.2\\columnwidth}|>{\\raggedright}p{0.06\\columnwidth}|>{\\raggedright}p{0.07\\columnwidth}|p{0.63\\columnwidth}|\n\n.. cssclass:: longtable/g' plugins/"${PLUG}".rst

done
popd

for y in "$TMP_FOLDER"/*.rst; do
    cp "$y" "$DOC_FOLDER/source" || exit 1
done
for z in "$TMP_FOLDER"/plugins/*.rst; do
    # Replace custom link, example: [linux](|html::/guide/linux.html#linux||rst::linux<Linux>|)
    # removes html::* and convert rst::* to a proper RST link, example: :ref:`linux<Linux>`
    $SED -i 's/<|html::.*||/|/g;s/|rst::\(.*\)|>.*__/:ref:\`\1\`/g' "${z}"

    # We insert rst-style refs in markdown as :ref:`label`, and pandoc
    # transforms these to :ref:``label`` - let's remove those double quotes
    # see sources/plugins/net.sf.openfx.MergeIn.rst for an example
    $SED -i 's/:ref:``\([^`]*\)``/:ref:`\1`/g' "${z}"
    
    cp "$z" "$DOC_FOLDER/source/plugins" || exit 1
done

for x in "$TMP_FOLDER"/plugins/*.png; do 
    cp "$x" "$DOC_FOLDER/source/plugins" || exit 1
done

echo rm -rf "$TMP_FOLDER"

# Local Variables:
# indent-tabs-mode: nil
# sh-basic-offset: 4
# sh-indentation: 4
# End:
