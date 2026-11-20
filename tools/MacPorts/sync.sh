#!/bin/sh
dir=`dirname $0`
mp=/opt/MacPorts-Natron
rsync -avh --delete-after "$dir"/ "$mp"/ && (cd "$mp" && portindex) &&  echo "Synced $dir to $mp"
