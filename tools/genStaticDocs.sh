#!/bin/sh
set -x

PANDOC="$HOME/.cabal/bin/pandoc"
NATRON_BIN=${1:-}
TMP_FOLDER=${2:-}
DOC_FOLDER=${3:-}

if [ -z "$NATRON_BIN" ] || [ -z "$TMP_FOLDER" ] || [ -z "$DOC_FOLDER" ]; then
  echo "Usage: script NATRON_RENDERER TMP_FOLDER DOC_FOLDER"
  exit 1
fi

if [ ! -d "$TMP_FOLDER" ]; then
  mkdir -p "$TMP_FOLDER" || exit 1
fi
rm -rf "$TMP_FOLDER"/* || true
if [ ! -d "$TMP_FOLDER/plugins" ]; then
  mkdir -p "$TMP_FOLDER/plugins" || exit 1
fi
if [ ! -d "$DOC_FOLDER" ]; then
  mkdir -p "$DOC_FOLDER" || exit 1
fi
if [ ! -d "$DOC_FOLDER/source/plugins" ]; then
  mkdir -p "$DOC_FOLDER/source/plugins" || exit 1
fi

touch dummy.ntp
"$NATRON_BIN" --export-docs "$TMP_FOLDER" dummy.ntp
cd "$TMP_FOLDER" || exit 1

for i in *.md;do $PANDOC $i --columns=1000 -o `echo $i|sed 's/.md/.rst/'`;done
for x in plugins/*.md;do
  $PANDOC $x --columns=9000 -o `echo $x|sed 's/.md/.rst/'`
  PLUG=`echo $x|sed 's/.md//;s#plugins/##'`
  sed -i "1i.. _${PLUG}:\n" plugins/${PLUG}.rst
done

for y in *.rst; do
    cp "$TMP_FOLDER"/$y "$DOC_FOLDER/source/" || exit 1
done
for z in plugins/*.rst; do
  # Replace custom link, example: [linux](|html::/guide/linux.html#linux||rst::linux<Linux>|)
  # removes html::* and convert rst::* to a proper RST link, example: :ref:`linux<Linux>`
  sed -i 's/<|html::.*||/|/g;s/|rst::\(.*\)|>.*__/:ref:\`\1\`/g' ${z}

  cp "$TMP_FOLDER"/$z "$DOC_FOLDER/source/plugins/" || exit 1
done

for x in plugins/*.png; do 
cp "$TMP_FOLDER"/$x "$DOC_FOLDER/source/plugins/" || exit 1
done

cd  .. || exit 1

rm -rf $TMP_FOLDER dummy.ntp
