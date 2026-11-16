#!/bin/bash
# Runs INSIDE a macOS tart VM (cirruslabs image, user "admin" with passwordless sudo).
# Installs the Natron MacPorts SDK (same procedure as INSTALL_MACOS.md) and builds Natron.
# Driven from the host by natron-sdk.sh (image) and natron-build.sh (builds).
#
# Expects (synced from the host by tart-common.sh):
#   ~/Documents/third_party/Natron           Natron checkout (overlay + jenkins scripts)
#   ~/Documents/third_party/osmesa-install   osmesa-install checkout
#
# Usage: provision.sh [STEP...]   (default: all SDK steps; finished steps are skipped)
#   SDK steps: disk xcode macports overlay sync pins osmesa compiler tools libs python_qt dmgtools
#   env:       REDO="step..." re-runs steps already marked done; PROXY=socks5h://... for internet access
#   others:    sync (re-publish the overlay), update (refresh an existing SDK), cleanup (shrink the image), build (build Natron)
# Logs:  ~/provision-logs/<step>.log ; done markers in ~/provision-logs/<step>.done
set -euo pipefail

SRC=$HOME/Documents/third_party
NATRON=$SRC/Natron
OSMESA_SRC=$SRC/osmesa-install
WORKSPACE=$HOME/Development/workspace
LOGDIR=$HOME/provision-logs
MACPORTS_VERSION=2.12.7
JOBS=${MKJOBS:-$(sysctl -n hw.ncpu)}
export PATH=/opt/local/bin:/opt/local/sbin:$PATH

os_major=$(uname -r | cut -d. -f1)
if   [ "$os_major" -ge 25 ]; then clang_version=22
elif [ "$os_major" -ge 24 ]; then clang_version=18
elif [ "$os_major" -gt 10 ]; then clang_version=17
else                              clang_version=11; fi

# Internet access through the SOCKS proxy provided by the host's SSH connection, if any
# (curl, git, MacPorts and osmesa-install all honor ALL_PROXY; sudo needs it passed explicitly).
PROXY=${PROXY:-}
if [ -n "$PROXY" ]; then
    export ALL_PROXY=$PROXY all_proxy=$PROXY NO_PROXY=localhost,127.0.0.1
    SUDO="sudo env ALL_PROXY=$PROXY NO_PROXY=localhost,127.0.0.1"
else
    SUDO=sudo
fi
HTTP_PROXY_PORT=8118

# pip can't use a SOCKS proxy (bug in its vendored urllib3), but it handles HTTP proxies:
# run privoxy on the guest's loopback, forwarding to the SOCKS proxy, for the build scripts' pip.
start_http_proxy() {
    [ -n "$PROXY" ] || return 0
    command -v privoxy > /dev/null || { echo "privoxy is missing (run: natron-sdk.sh update tools)"; return 1; }
    if ! nc -z 127.0.0.1 $HTTP_PROXY_PORT 2> /dev/null; then
        local socks=${PROXY#*://}
        mkdir -p "$HOME/.privoxy"
        printf 'listen-address 127.0.0.1:%s\nforward-socks5t / %s .\ntoggle 0\nenable-remote-toggle 0\nenable-edit-actions 0\n' \
            "$HTTP_PROXY_PORT" "$socks" > "$HOME/.privoxy/config"
        privoxy --no-daemon "$HOME/.privoxy/config" > "$HOME/.privoxy/log" 2>&1 &
        sleep 2
    fi
    export http_proxy=http://127.0.0.1:$HTTP_PROXY_PORT https_proxy=http://127.0.0.1:$HTTP_PROXY_PORT
    export HTTP_PROXY=$http_proxy HTTPS_PROXY=$https_proxy
}
PORT="port_retry"
# large downloads through the SOCKS tunnel are sometimes cancelled: retry port commands
port_retry() {
    local i
    for i in 1 2 3; do
        $SUDO port -v -N "$@" && return 0
        echo "port $* failed (attempt $i)"
        sleep 10
    done
    return 1
}
nopy="-python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314"

# Grow the APFS container to the (possibly enlarged) virtual disk (see tart docs, "disk resizing").
step_disk() {
    yes | sudo diskutil repairDisk disk0 || true
    sudo diskutil apfs resizeContainer disk0s2 0 || true
    df -h /
}

step_xcode() {
    local xcode=/Applications/Xcode.app
    [ -d "$xcode" ] || xcode=$(ls -d /Applications/Xcode*.app | sort -V | tail -1)
    sudo xcode-select -s "$xcode/Contents/Developer"
    sudo xcodebuild -license accept
    xcodebuild -version
}

# Install MacPorts $MACPORTS_VERSION from the official .pkg (also upgrades an older base).
step_macports() {
    if [ -x /opt/local/bin/port ] && [ "$(port version | awk '{print $2}')" = "$MACPORTS_VERSION" ]; then
        port version; return
    fi
    local mac_major name pkg dl
    mac_major=$(sw_vers -productVersion | cut -d. -f1)
    case "$mac_major" in
        13) name=Ventura ;; 14) name=Sonoma ;; 15) name=Sequoia ;; 26) name=Tahoe ;; 27) name=GoldenGate ;;
        *) echo "unknown macOS $mac_major: set the MacPorts pkg name"; exit 1 ;;
    esac
    pkg=MacPorts-${MACPORTS_VERSION}-${mac_major}-${name}.pkg
    dl=$(mktemp -d /tmp/macports-dl.XXXX)
    curl -fsSL -o "$dl/$pkg" "https://github.com/macports/macports-base/releases/download/v${MACPORTS_VERSION}/$pkg"
    curl -fsSL -o "$dl/chk.txt" "https://github.com/macports/macports-base/releases/download/v${MACPORTS_VERSION}/MacPorts-${MACPORTS_VERSION}.chk.txt"
    grep -q "SHA256($pkg)= $(shasum -a 256 "$dl/$pkg" | cut -d' ' -f1)" "$dl/chk.txt" ||
        grep -q "MD5($pkg)= $(md5 -q "$dl/$pkg")" "$dl/chk.txt" || { echo "checksum mismatch for $pkg"; exit 1; }
    # Developer ID signature of the MacPorts release manager (notarization can't be checked
    # online from a guest without direct internet access)
    pkgutil --check-signature "$dl/$pkg" > "$dl/sig.txt" || true
    grep -q "signed by a developer certificate issued by Apple for distribution" "$dl/sig.txt" &&
        grep -q "Developer ID Installer: Joshua Root (QTA3A3B7F3)" "$dl/sig.txt" || { cat "$dl/sig.txt"; echo "bad signature"; exit 1; }
    sudo installer -pkg "$dl/$pkg" -target /
    port version
}

# Fetch the ports tree over HTTPS (works through the SOCKS proxy; rsync would not).
ports_https_source() {
    local conf=/opt/local/etc/macports/sources.conf
    if grep -q '^rsync://.*\[default\]' $conf; then
        sudo perl -pi -e 's|^rsync://\S+(\s+\[default\])|https://distfiles.macports.org/ports.tar.gz$1|' $conf
    fi
    # transient download/signature failures happen (e.g. HTTP/2 stream errors): retry
    local i
    for i in 1 2 3; do
        $SUDO port -v sync && return 0
        echo "port sync failed (attempt $i)"
        sleep 20
    done
    return 1
}

step_overlay() {
    ports_https_source
    (cd "$NATRON/tools/MacPorts" && ./check.sh) | tee "$LOGDIR/check.txt"
    if grep -q "is obsolete\|is missing" "$LOGDIR/check.txt"; then
        echo "WARNING: overlay is out of date with this upstream tree (see $LOGDIR/check.txt)"
    fi
    sudo mkdir -p /opt/MacPorts-Natron
    sudo chown "$(id -un):staff" /opt/MacPorts-Natron
    grep -q "^file:///opt/MacPorts-Natron" /opt/local/etc/macports/sources.conf ||
        sudo perl -0pi -e 's|^(\S+\s+\[default\])|file:///opt/MacPorts-Natron\n$1|m' /opt/local/etc/macports/sources.conf
    step_sync
    port file openimageio | grep -q '^/opt/MacPorts-Natron/' || { echo "overlay not used"; exit 1; }
}

step_osmesa() {
    if [ -f /opt/osmesa/lib/libMangledOSMesa32.a ] && [ -x /opt/llvm/bin/llvm-config ]; then return; fi
    sudo mkdir -p /opt/osmesa /opt/llvm
    sudo chown "$(id -un):staff" /opt/osmesa /opt/llvm
    $PORT install python27 cmake autoconf automake libtool pkgconfig gsed
    sudo port select --set python python27
    mkdir -p "$OSMESA_SRC/build"
    (cd "$OSMESA_SRC/build" && env LLVM_BUILD=1 MKJOBS="$JOBS" ../osmesa-install.sh)
    sudo port select --set python none
    [ -f /opt/osmesa/lib/libMangledOSMesa32.a ] && [ -x /opt/llvm/bin/llvm-config ]
}

step_compiler() {
    sudo port select clang none || true
    $PORT install clang-${clang_version}
    sudo port setrequested clang-${clang_version}
    printf 'int main(){}\n' > /tmp/t.cpp
    clang++-mp-${clang_version} -stdlib=libc++ -fopenmp /tmp/t.cpp -o /tmp/t
    otool -L /tmp/t
}

step_tools() {
    $PORT install gsed gawk coreutils findutils keychain tmux gzip bash wget bison flex doxygen \
        texinfo ncdu1 meson ninja help2man unzip tcl expect
    $PORT install privoxy     # HTTP->SOCKS proxy for pip in VMs without direct internet access
}

step_libs() {
    $PORT install dbus
    $PORT install glib2 -x11
    $PORT install libjpeg-turbo libpng tiff expat libmng boost
    $PORT install cairo -x11
    $PORT install x264 x265
    $PORT install libvpx +highbitdepth
    $PORT install libraw +gpl2
    $PORT uninstall ilmbase opencv4 field3d || true
    $PORT install -f openexr
    $PORT install minizip-ng pystring
    [ -e /opt/local/include/minizip-ng ] || sudo ln -s . /opt/local/include/minizip-ng
    sudo sed -i '' -e 's/^find_dependency(zstd/#find_dependency(zstd/' /opt/local/lib/cmake/minizip-ng/minizip-ng-config.cmake
    $PORT install opencolorio  -quartz $nopy
    $PORT install opencolorio1 -quartz $nopy
    $PORT install seexpr seexpr211
    $PORT install librsvg -quartz
    $PORT install poppler librevenge libcdr-0.1 sox libzip
    sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
    $PORT install ImageMagick +natron -x11
    $PORT install fftw-3 -gfortran fftw-3-single -gfortran
    $PORT install ffmpeg +gpl2 +highbitdepth +natronmini
    $PORT install openimageio +freetype +natron +opencolorio +openjpeg +python310 -python312 -python313 -python314
}

# pip can't use a SOCKS proxy (vendored urllib3 bug), so fetch the wheel with curl,
# check it against the expected sha256, and install it offline.
pip_install_wheel() { # project version sha256
    local json url file sha d
    d=$(mktemp -d /tmp/wheel.XXXX)
    json=$(curl -fsSL "https://pypi.org/pypi/$1/$2/json")
    read -r url sha < <(printf '%s' "$json" | python3.10 -c 'import json,sys
u=[u for u in json.load(sys.stdin)["urls"] if u["packagetype"]=="bdist_wheel"][0]
print(u["url"], u["digests"]["sha256"])')
    file=$d/$(basename "$url")
    curl -fsSL -o "$file" "$url"
    [ "$sha" = "$3" ] || { echo "PyPI lists sha256 $sha for $1 $2, expected $3"; return 1; }
    [ "$(shasum -a 256 "$file" | cut -d" " -f1)" = "$3" ] || { echo "sha256 mismatch for $file"; return 1; }
    sudo pip-3.10 install --no-index --no-deps "$file"
    python3.10 -c "import importlib.metadata as m; assert m.version('$1') == '$2'"
}

step_python_qt() {
    $PORT install py310-pip
    $PORT install openblas
    $PORT install py310-numpy -gfortran
    $PORT install py310-sphinx py310-sphinx_rtd_theme
    $PORT install qt5 qt5-qtwebengine
    $PORT install py310-pyside2
    sudo port select --set python python310
    sudo port select --set python3 python310
    sudo port select --set sphinx py310-sphinx
    pip_install_wheel QtPy 1.11.2 83c502973e9fdd7b648d8267a421229ea3d9a0651c22e4c65a4d9228479c39b6
    local py=/opt/local/Library/Frameworks/Python.framework/Versions/3.10
    [ -e $py/lib/python3.10/site-packages/shiboken2_generator/shiboken2-3.10 ] ||
        sudo ln -s shiboken2 $py/lib/python3.10/site-packages/shiboken2_generator/shiboken2-3.10
    sudo ln -sf shiboken2-3.10 /opt/local/bin/shiboken2
    [ -e /opt/local/Library/lib ] || sudo ln -s Frameworks /opt/local/Library/lib
    [ -f $py/lib/python3.10/site-packages/shiboken2_generator/shiboken2-3.10 ]
}

# Idempotent fixes of installed files that a port upgrade may undo, and the port selections
# (step_osmesa temporarily selects python27). Never marked done: re-run on every create/update.
step_fixups() {
    [ -e /opt/local/include/minizip-ng ] || sudo ln -s . /opt/local/include/minizip-ng
    sudo sed -i '' -e 's/^find_dependency(zstd/#find_dependency(zstd/' /opt/local/lib/cmake/minizip-ng/minizip-ng-config.cmake
    sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
    sudo port select --set python python310
    sudo port select --set python3 python310
    sudo port select --set sphinx py310-sphinx
    local py=/opt/local/Library/Frameworks/Python.framework/Versions/3.10
    [ -e $py/lib/python3.10/site-packages/shiboken2_generator/shiboken2-3.10 ] ||
        sudo ln -s shiboken2 $py/lib/python3.10/site-packages/shiboken2_generator/shiboken2-3.10
    sudo ln -sf shiboken2-3.10 /opt/local/bin/shiboken2
    [ -e /opt/local/Library/lib ] || sudo ln -s Frameworks /opt/local/Library/lib
    if [ -x /opt/local/bin/dmgbuild-3.10 ]; then sudo ln -sf dmgbuild-3.10 /opt/local/bin/dmgbuild; fi
}

step_dmgtools() {
    $PORT install py310-dmgbuild
    # the port only installs dmgbuild-3.10 (no port select group)
    [ -e /opt/local/bin/dmgbuild ] || sudo ln -s dmgbuild-3.10 /opt/local/bin/dmgbuild
    command -v dmgbuild || { echo "dmgbuild not in PATH"; ls /opt/local/bin | grep -i dmgbuild || true; exit 1; }
    dmgbuild --help > /dev/null
    command -v oiiotool && command -v identify
}


# Re-publish the overlay (used by builds, in case tools/MacPorts changed since the image was made).
# This makes Portfile changes visible to MacPorts; installed ports are only rebuilt by "update".
step_sync() {
    (cd "$NATRON/tools/MacPorts" && ./sync.sh) > "$LOGDIR/sync-portindex.txt" 2>&1
    tail -5 "$LOGDIR/sync-portindex.txt"
    grep -q "Ports failed:[[:space:]]*0" "$LOGDIR/sync-portindex.txt" || { echo "portindex failed on the overlay"; return 1; }
}

# Ports pinned in the overlay (older than upstream): MacPorts never downgrades by itself,
# so reinstall them (same variants) when the installed version differs from the overlay's.
PINNED_PORTS="libraw"
step_pins() {
    local p want have variants vfile
    for p in $PINNED_PORTS; do
        vfile=$LOGDIR/pins-$p.variants
        have=$(port -q installed "$p" 2>/dev/null | awk '/\(active\)/{print $2}')
        if [ -z "$have" ]; then
            # an interrupted reinstall: install it again with the saved variants
            if [ -f "$vfile" ]; then
                variants=$(cat "$vfile")
                echo "$p: not installed, reinstalling with$variants"
                $PORT install "$p" $variants
            fi
            continue
        fi
        want=$(port -q info --version "$p")
        case "$have" in
            "@$want"_*) echo "$p: $have matches the overlay ($want)" ;;
            *)
                variants=$(echo "$have" | sed -e 's/^@[^+]*//' -e 's/+/ +/g')
                echo "$variants" > "$vfile"
                echo "$p: installed $have, overlay has $want: reinstalling with$variants"
                $SUDO port -v -N -f uninstall "$p"
                $PORT install "$p" $variants
                ;;
        esac
        rm -f "$vfile"
    done
}

# Update an existing SDK image: refresh MacPorts and the overlay, upgrade outdated ports.
step_update() {
    # resumable: "natron-sdk.sh create" re-runs the update while this marker exists
    date '+%F %T' > "$LOGDIR/update.pending"
    # MacPorts base: install the .pkg of MACPORTS_VERSION if it differs from the installed one
    step_macports
    ports_https_source
    (cd "$NATRON/tools/MacPorts" && ./check.sh) | tee "$LOGDIR/check.txt"
    if grep -q "is obsolete\|is missing" "$LOGDIR/check.txt"; then
        echo "WARNING: overlay is out of date with upstream (see $LOGDIR/check.txt); reintegrate it on the host first"
    fi
    step_sync
    # pinned ports first, so that their dependents are upgraded against the right versions
    step_pins
    local outdated
    outdated=$(port -q echo outdated | awk '{print $1}')
    for p in $outdated; do
        echo "===== upgrading $p"
        $PORT upgrade "$p"
    done
    # FFmpeg 9's default +vmaf pulls libvmaf into the bundle: +natronmini no longer selects it,
    # but "upgrade" keeps the installed variants, so reinstall an ffmpeg that has +vmaf.
    # The marker makes an interrupted reinstall resumable (ffmpeg uninstalled but not reinstalled).
    if [ -f "$LOGDIR/ffmpeg-novmaf.pending" ] || port -q installed ffmpeg 2>/dev/null | grep '(active)' | grep -q '+vmaf'; then
        echo "===== reinstalling ffmpeg without +vmaf"
        touch "$LOGDIR/ffmpeg-novmaf.pending"
        if port -q installed ffmpeg 2>/dev/null | grep -q .; then
            $PORT -f uninstall ffmpeg
        fi
        $PORT install ffmpeg +gpl2 +highbitdepth +natronmini
        rm -f "$LOGDIR/ffmpeg-novmaf.pending"
        # libvmaf is no longer needed by anything
        $PORT -N uninstall libvmaf 2>/dev/null || true
    fi
    # rebuild OSMesa/LLVM only if missing (delete /opt/osmesa/lib to force a rebuild)
    step_osmesa
    rm -f "$LOGDIR/update.pending"
}

# Shrink the image (build directories, downloaded sources), and remove the sources synced
# from the host so that a pushed image only contains what the steps installed.
# The port archives in /opt/local/var/macports/software are kept (needed to re-activate ports).
step_cleanup() {
    sudo port -q clean --all installed || true
    sudo rm -rf /opt/local/var/macports/distfiles/* /opt/local/var/macports/build/*
    rm -rf "$OSMESA_SRC/build" "$NATRON" "$OSMESA_SRC"
    df -h /
}

step_build() {
    start_http_proxy
    mkdir -p "$WORKSPACE"
    cd "$NATRON/tools/jenkins"
    env NOUPDATE=1 WORKSPACE="$WORKSPACE" DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL \
        GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH="${GIT_BRANCH:-RB-2.6}" \
        UNIT_TESTS="${UNIT_TESTS:-false}" BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 \
        COMPILE_TYPE=release MKJOBS="$JOBS" \
        ./launchBuildMain.sh
    # launchBuildMain.sh can exit 0 after a failed sub-step: require the DMG
    ls -l "$WORKSPACE/builds_archive/natron_github_RB2/1/"
    ls "$WORKSPACE"/builds_archive/natron_github_RB2/1/*.dmg
}

SDK_STEPS="disk xcode macports overlay sync pins osmesa compiler tools libs python_qt dmgtools fixups"
# steps that are never marked done (they are meant to be re-run)
NO_MARKER_STEPS="disk sync pins update fixups cleanup build"
mkdir -p "$LOGDIR"
# REDO="step ...": forget that these steps were done (e.g. after fixing a step)
for s in ${REDO:-}; do rm -f "$LOGDIR/$s.done"; done
steps=${*:-$SDK_STEPS}
# an interrupted "update" is finished before the other steps (natron-sdk.sh create resumes it)
if [ -z "${*:-}" ] && [ -f "$LOGDIR/update.pending" ]; then steps="update $steps"; fi
for s in $steps; do
    if [ -f "$LOGDIR/$s.done" ]; then echo "== $s: already done"; continue; fi
    mark=1; case " $NO_MARKER_STEPS " in *" $s "*) mark=0 ;; esac
    echo "== $s: started $(date '+%F %T') (log: $LOGDIR/$s.log)"
    # not in an "if": bash ignores set -e inside a tested subshell, which would hide failures
    set +e
    ( set -ex; "step_$s" ) > "$LOGDIR/$s.log" 2>&1
    rc=$?
    set -e
    if [ "$rc" = 0 ]; then
        [ "$mark" = 0 ] || date '+%F %T' > "$LOGDIR/$s.done"
        echo "== $s: OK $(date '+%F %T')"
    else
        echo "== $s: FAILED $(date '+%F %T'), last lines of $LOGDIR/$s.log:"
        tail -30 "$LOGDIR/$s.log"
        exit 1
    fi
done
echo "== all requested steps done"
