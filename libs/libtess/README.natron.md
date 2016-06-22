libtess was taken from https://cgit.freedesktop.org/mesa/glu/ at commit 0bf42e41c8b63fc2488dd8d41f696310b5a5a6a7

To update:

Clone the repository...

    git clone git://anongit.freedesktop.org/mesa/glu

    cd glu

    patch -p1 /path-to-natron/libs/libtess/libtess-natron.patch

    cp glu/src/libtess/*  /path-to-natron/libs/libtess/