#!/bin/sh
#
# Create repo file for breakdown
#

REPORTS=/home/socorro/reports
cd $REPORTS || exit 1
echo > repo.txt
for i in *.json; do
  FDATE=`date -r ${i} +%F`
  MD5=`md5sum ${i}|awk '{print $1}'`
  echo "$FDATE $MD5 $i" >> repo.txt
done

