#!/bin/sh
#
# Create repo file for breakdown
#

REPORTS=/home/socorro/reports
cd $REPORTS || exit 1
echo > repo.txt
for i in *.json; do
  MD5=`md5sum ${i}|awk '{print $1}'`
  echo "$MD5 $i" >> repo.txt
done

