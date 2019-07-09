#!/bin/bash
# see https://github.com/videolan/x265/blob/master/build/msys/multilib.sh
# cmake also launches the build command during configuration, so the
# libraries may not be built yet.

set -e
#set -x

if [ "$ARCH" != "x86_64" ]; then
    # no multilib on i386 or ppc
    exec "$BUILD" "$@"
fi

if [ -d ../10bit-$ARCH ]; then
  (cd ../10bit-$ARCH && "$BUILD" "$@")
  [ -f ../10bit-$ARCH/libx265.a ] && mv ../10bit-$ARCH/libx265.a libx265_main10.a
fi
if [ -d ../12bit-$ARCH ]; then
  (cd ../12bit-$ARCH && "$BUILD" "$@")
  [ -f ../12bit-$ARCH/libx265.a ] && mv ../12bit-$ARCH/libx265.a libx265_main12.a
fi
echo "$BUILD" "$@"
"$BUILD" "$@"
if [ -f libx265.a ]; then
  mv libx265.a libx265_main.a
  libtool -static -o libx265.a libx265_main.a libx265_main10.a libx265_main12.a
fi
