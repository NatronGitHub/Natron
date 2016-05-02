#!/bin/sh
for i in /opt/local/lib/lib*[^0-9].dylib; do ls `echo $i|sed -e s/dylib/a/` > /dev/null; done

