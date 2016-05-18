#!/bin/sh
NATRON_BIN=${1:-}
TMP_FOLDER=${2:-}
DOC_FOLDER=${3:-}

if [ -z "$NATRON_BIN" ] || [ -z "$TMP_FOLDER" ] || [ -z "$DOC_FOLDER" ]; then
  echo "Usage: script NATRON_RENDERER TMP_FOLDER DOC_FOLDER"
  exit 1
fi

touch dummy.ntp
mkdir -p "$TMP_FOLDER" || exit 1
./"$NATRON_BIN" --export-docs "$TMP_FOLDER" dummy.ntp
cd "$TMP_FOLDER" || exit 1

for i in *.md;do pandoc $i --columns=1000 -o `echo $i|sed 's/.md/.rst/'`;done

for y in *.rst; do
if [ ! -f "$DOC_FOLDER/source/${y}" ]; then
  cp $y "$DOC_FOLDER/source/" || exit 1
else
  DIFF=`diff ${DOC_FOLDER}/source/${y} ${y}`
  if [ ! -z "$DIFF" ]; then
    cp $y "$DOC_FOLDER/source/" || exit 1
  fi
fi
done

cd  .. || exit 1

rm -rf $TMP_FOLDER dummy.ntp
