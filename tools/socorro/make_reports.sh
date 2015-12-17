#!/bin/sh
#
# Create reports for breakdown
#

DUMPS=/home/socorro/crashes
SCAN=/data/socorro/stackwalk/bin/stackwalker
SYMBOLS=/home/socorro/symbols
REPORTS=/home/socorro/reports

JSON=`find $DUMPS | sed '/dump/!d'`
if [ "$JSON" != "" ]; then
  for i in $JSON; do
    REPORT=`echo $i|sed 's/\.dump//;s@.*/@@'`
    HEADER=`echo $i|sed 's/\.dump/\.json/'`
    if [ ! -f "$REPORTS/$REPORT.json" ]; then
      cat $HEADER | sed s'/.$/,/' > $REPORTS/$REPORT.json
      $SCAN $i $SYMBOLS | sed 's/^.//' >> $REPORTS/$REPORT.json
    fi
  done
fi

