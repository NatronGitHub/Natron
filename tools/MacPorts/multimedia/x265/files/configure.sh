#!/bin/bash
set -e
#set -x

if [ "$ARCH" != "x86_64" ]; then
    exec "$CONFIGURE" "$@"
fi

# gcc version 4.8 or higher required for -DENABLE_HDR10_PLUS=ON
args=( -DLINKED_10BIT=ON -DLINKED_12BIT=ON -DEXTRA_LINK_FLAGS=-L. -DEXTRA_LIB='x265_main10.a;x265_main12.a' )
high_bit_depth_args=( -DHIGH_BIT_DEPTH=ON -DEXPORT_C_API=OFF -DENABLE_SHARED=OFF -DENABLE_CLI=OFF )

[ -d ../10bit-$ARCH ] || mkdir ../10bit-$ARCH
(cd ../10bit-$ARCH && "$CONFIGURE" "$@" "${high_bit_depth_args[@]}")

[ -d ../12bit-$ARCH ] || mkdir ../12bit-$ARCH
(cd ../12bit-$ARCH && "$CONFIGURE" "$@" "${high_bit_depth_args[@]}" -DMAIN12=ON)

"$CONFIGURE" "$@" "${args[@]}"
