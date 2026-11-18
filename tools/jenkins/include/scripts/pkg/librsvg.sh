#!/bin/bash

# Install librsvg (without vala support)
# see http://www.linuxfromscratch.org/blfs/view/systemd/general/librsvg.html
if [ "${ROCKY:-0}" -ge 9 ]; then
    # Rust comes from rust-toolset (see the Dockerfile generation in build-Linux-sdk.sh).
    # 2.52 or later is needed by openfx-arena's ReadSVG for the exact document size
    # (rsvg_handle_get_intrinsic_size_in_pixels). 2.58 is the last series built with autotools.
    # Its tarball doesn't vendor the Rust crates: cargo downloads them.
    LIBRSVG_VERSION=2.58.5
else
    LIBRSVG_VERSION=2.40.20 # 2.41 requires rust
fi
LIBRSVG_VERSION_SHORT=${LIBRSVG_VERSION%.*}
LIBRSVG_TAR="librsvg-${LIBRSVG_VERSION}.tar.xz"
LIBRSVG_SITE="https://download.gnome.org/sources/librsvg/${LIBRSVG_VERSION_SHORT}"
if download_step; then
    download "$LIBRSVG_SITE" "$LIBRSVG_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_SVG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/librsvg* "$SDK_HOME"/lib/librsvg* "$SDK_HOME"/lib/pkgconfig/librsvg* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/librsvg-2.0.pc" ] || [ "$(pkg-config --modversion librsvg-2.0)" != "$LIBRSVG_VERSION" ]; }; }; then
    start_build
    if version_gt "$LIBRSVG_VERSION_SHORT" 2.40; then
        # librsvg 2.41 requires rust
        if ! command -v cargo > /dev/null; then
            if [ ! -s "$HOME/.cargo/env" ]; then
                (>&2 echo "Error: librsvg requires rust. Please install rust by executing:")
                (>&2 echo "$SDK_HOME/bin/curl https://sh.rustup.rs -sSf | sh")
                exit 1
            fi
            source "$HOME/.cargo/env"
        fi
        if [ "$ARCH" = "x86_64" ]; then
            RUSTFLAGS="-C target-cpu=x86-64"
        elif [ "$ARCH" = "i686" ]; then
            RUSTFLAGS="-C target-cpu=i686"
        else
            RUSTFLAGS="-C target-cpu=generic"
        fi
        export RUSTFLAGS
    fi
    untar "$SRC_PATH/$LIBRSVG_TAR"
    pushd "librsvg-${LIBRSVG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-gtk-doc --disable-static --enable-shared --disable-introspection --disable-vala --disable-pixbuf-loader
    make -j${MKJOBS}
    make install
    popd
    rm -rf "librsvg-${LIBRSVG_VERSION}"
    end_build
fi
