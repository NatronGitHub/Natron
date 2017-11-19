#!/bin/bash
set -e
#set -x

if [ "$ARCH" != "x86_64" ]; then
    exec "$BUILD" "$@"
fi

(cd ../10bit-$ARCH && "$BUILD" "$@")
mv ../10bit-$ARCH/libx265.a libx265_main10.a
(cd ../12bit-$ARCH && "$BUILD" "$@")
mv ../12bit-$ARCH/libx265.a libx265_main12.a
"$BUILD" "$@"
mv libx265.a libx265_main.a
libtool -static -o libx265.a libx265_main.a libx265_main10.a libx265_main12.a
