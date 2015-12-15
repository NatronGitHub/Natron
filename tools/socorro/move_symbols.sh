#!/bin/sh
#
# Script that moves uploaded symbols
#
TMP=/home/tmp
DEST=/home/socorro/symbols
cd $TMP || exit 1
for i in *.sym; do
  SYM_ID=`head -n1 ${i}|awk '{print $4}'`
  SYM_APP=`head -n1 ${i}|awk '{print $5}'`
  if [ "$SYM_ID" != "" ] && [ "$SYM_APP" != "" ]; then
    if [ ! -d "$DEST/$SYM_APP/$SYM_ID" ]; then
      mkdir -p $DEST/$SYM_APP/$SYM_ID || exit 1
      mv $i $DEST/$SYM_APP/$SYM_ID/${SYM_APP}.sym || exit 1
      echo "Added symbols for $i"
    else
      echo "Symbols already exists for $i"
    fi
  fi
done
