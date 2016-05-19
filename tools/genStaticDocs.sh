#!/bin/sh
set -x

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
mkdir -p "$TMP_FOLDER" || exit 1
"$NATRON_BIN" --export-docs "$TMP_FOLDER" dummy.ntp
cd "$TMP_FOLDER" || exit 1

for i in *.md;do pandoc $i --columns=1000 -o `echo $i|sed 's/.md/.rst/'`;done
for i in plugins/*.md;do pandoc $i --columns=1000 -o `echo $i|sed 's/.md/.rst/'`;done

for y in *.rst; do
if [ ! -f "$DOC_FOLDER/source/${y}" ]; then
  cp "$TMP_FOLDER"/$y "$DOC_FOLDER/source/" || exit 1
else
  DIFF=`diff ${DOC_FOLDER}/source/${y} ${TMP_FOLDER}/${y}`
  if [ ! -z "$DIFF" ]; then
    cp "$TMP_FOLDER"/$y "$DOC_FOLDER/source/" || exit 1
  fi
fi
done
for z in plugins/*.rst; do
if [ ! -f "$DOC_FOLDER/source/${z}" ]; then
  cp "$TMP_FOLDER"/$z "$DOC_FOLDER/source/plugins/" || exit 1
else
  DIFF=`diff ${DOC_FOLDER}/source/${z} ${TMP_FOLDER}/${z}`
  if [ ! -z "$DIFF" ]; then
    cp "$TMP_FOLDER"/$z "$DOC_FOLDER/source/plugins/" || exit 1
  fi
fi
done
for x in plugins/*.png; do 
cp "$TMP_FOLDER"/$x "$DOC_FOLDER/source/plugins/" || exit 1
done

cd  .. || exit 1

rm -rf $TMP_FOLDER dummy.ntp
