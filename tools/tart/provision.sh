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
#   SDK steps: disk nobrew clt macports macports_conf overlay sync pins osmesa compiler tools libs python_qt
#              dmgtools rebuild_target fixups
#   env:       REDO="step..." re-runs steps already marked done; PROXY=socks5h://... for internet access
#   others:    sync (re-publish the overlay), update (refresh an existing SDK), cleanup (shrink the image), build (build Natron)
# Logs:  ~/provision-logs/<step>.log ; done markers in ~/provision-logs/<step>.done
set -euo pipefail

# This script changes the machine it runs on: it removes Homebrew and the Command Line Tools,
# sets the system proxy, rewrites ~/.zprofile and the git configuration. Refuse to run anywhere
# but in a tart VM driven by natron-sdk.sh/natron-build.sh (vm_start writes /tmp/natron-tart-id).
if [ "$(sysctl -n kern.hv_vmm_present 2> /dev/null)" != 1 ] ||
       { [ "$(id -un)" != admin ] && [ ! -f /tmp/natron-tart-id ]; }; then
    echo "provision.sh: not in a Natron tart VM (kern.hv_vmm_present != 1, or neither user admin nor /tmp/natron-tart-id): refusing to run" >&2
    exit 1
fi

SRC=$HOME/Documents/third_party
NATRON=$SRC/Natron
OSMESA_SRC=$SRC/osmesa-install
WORKSPACE=$HOME/Development/workspace
LOGDIR=$HOME/provision-logs
MACPORTS_VERSION=2.12.7
JOBS=${MKJOBS:-$(sysctl -n hw.ncpu)}
# Oldest macOS supported by Natron's macOS release, and the Command Line Tools of the SDK image:
# tools/jenkins/macos-release.sh. The SDK image runs that macOS itself, with the Command Line
# Tools and their macOS 14 SDK, so MacPorts' binary archives (built for the macOS they were made
# on) can be used. Every port (their dylibs are bundled), OSMesa/LLVM and Natron itself are built
# for it (step_build sets NATRON_MACOS_MIN, see tools/jenkins/common.sh).
# shellcheck source=../jenkins/macos-release.sh
. "$NATRON/tools/jenkins/macos-release.sh"
macos_target=$NATRON_MACOS_RELEASE_MIN
CHECK_MINOS=$NATRON/tools/tart/check-minos.sh
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
# large downloads through the SOCKS tunnel are sometimes cancelled: retry port commands.
# MacPorts fetches binary archives in the background while it installs other ports, and when
# such a fetch fails it silently builds the port from source (e.g. llvm-18, hours instead of
# seconds, or qt5-qtwebengine, which can't be built without Xcode): so fetch the archives of the
# ports to install and of their dependencies first, with retries.
# The ports keep their variants: archivefetch leaves a work directory whose variants must match
# the install's (and those of the dependencies are removed afterwards).
prefetch_archives() { # [-n] install|upgrade [-f] port [+variant -variant]...
    local p i expr="" deps=""
    for p in "$@"; do
        case $p in
            -s) return 0 ;;   # built from source
            install|upgrade|-f|-n|--force|--enforce-variants) ;;
            [+-]*) expr="$expr $p" ;;
            *) expr="$expr $p"; deps="$deps rdepof:$p" ;;
        esac
    done
    [ -n "$deps" ] || return 0
    expr="$expr$deps"
    for i in 1 2 3; do
        # shellcheck disable=SC2086
        if $SUDO port -N archivefetch $expr; then break; fi
        echo "port archivefetch failed (attempt $i)"
        sleep 10
    done
    # archivefetch leaves work directories with the default variants of the dependencies, and
    # "install" propagates the requested variants to them (e.g. glib2 -x11 to glib2-bootstrap):
    # remove them (the fetched archives stay in MacPorts' incoming directory)
    # shellcheck disable=SC2086
    $SUDO port -q clean --work $expr > /dev/null 2>&1 || true
    return 0   # the install below retries, and builds what has no archive
}
port_retry() {
    local i
    case " $* " in *" install "*|*" upgrade "*) prefetch_archives "$@" ;; esac
    for i in 1 2 3; do
        $SUDO port -v -N "$@" && return 0
        echo "port $* failed (attempt $i)"
        sleep 10
    done
    return 1
}
nopy="-python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314"
# Python of the Qt5 SDK: 3.11 (Python version policy, see INSTALL_MACOS.md section 8; 3.10 is EOL)
pyver=3.11
pyv=${pyver/./}
# openimageio's python variants: only the SDK's (its Portfile defaults to the newest Python)
oiio_python_variants() {
    local v out=+python$pyv
    for v in 310 311 312 313 314; do [ "$v" = "$pyv" ] || out="$out -python$v"; done
    echo "$out"
}

# Grow the APFS container to the (possibly enlarged) virtual disk (see tart docs, "disk resizing").
step_disk() {
    yes | sudo diskutil repairDisk disk0 || true
    sudo diskutil apfs resizeContainer disk0s2 0 || true
    df -h /
}

# The cirruslabs base images come with Homebrew (/opt/homebrew, /etc/paths.d/homebrew, and
# "brew shellenv" in ~/.zprofile): MacPorts only, so remove it, keeping the tart guest agent
# (installed by Homebrew, needed by "tart exec"), which only links system libraries.
step_nobrew() {
    local agent=/usr/local/libexec/tart-guest-agent f
    if [ -e /opt/homebrew/bin/tart-guest-agent ]; then
        sudo mkdir -p /usr/local/libexec
        sudo cp "$(readlink -f /opt/homebrew/bin/tart-guest-agent)" "$agent"
        sudo chmod 755 "$agent"
    fi
    for f in /Library/LaunchAgents/org.cirruslabs.tart-guest-agent.plist /Library/LaunchDaemons/org.cirruslabs.tart-guest-daemon.plist; do
        [ -f "$f" ] || continue
        sudo perl -pi -e "s|/opt/homebrew/bin/tart-guest-agent|$agent|; s|:/opt/homebrew/bin||" "$f"
    done
    if grep -q /opt/homebrew /Library/LaunchAgents/org.cirruslabs.tart-guest-agent.plist /Library/LaunchDaemons/org.cirruslabs.tart-guest-daemon.plist 2>/dev/null; then
        echo "the tart guest agent still uses /opt/homebrew"; return 1
    fi
    # the running agent keeps its (deleted) binary until the next boot
    sudo rm -rf /opt/homebrew /etc/paths.d/homebrew "$HOME/.homebrew"
    # ~/.profile is a link to ~/.zprofile: keep its LANG, drop Homebrew, rbenv, node and pnpm
    if [ -f "$HOME/.zprofile" ]; then
        grep -v -E 'brew|HOMEBREW|rbenv|node@|PNPM' "$HOME/.zprofile" > "$HOME/.zprofile.new" || true
        mv "$HOME/.zprofile.new" "$HOME/.zprofile"
    fi
    # git-lfs came from Homebrew
    git config --global --remove-section filter.lfs 2>/dev/null || true
    cat "$HOME/.zprofile"
    if zsh -lc 'command -v brew'; then echo "brew is still on the login PATH"; return 1; fi
    [ ! -e /opt/homebrew ]
}

# The Command Line Tools with the SDK of the target macOS (14), and nothing else (no Xcode):
# CLT $CLT_VERSION has the macOS 14.4 SDK (default) and 13.3, and Apple clang 15.0.0. The base
# image has CLT 16.2 (macOS 15.2 and 14.5 SDKs), which is replaced. softwareupdate installs the
# CLT without an Apple ID when $CLT_FLAG exists; it uses the system proxy settings, set to the
# SOCKS proxy for the duration of the install.
CLT_VERSION=$NATRON_MACOS_CLT_VERSION
CLT_FLAG=/tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
system_proxy() { # on|off : the system SOCKS proxy (softwareupdate ignores ALL_PROXY); "off" always runs
    local s
    [ -n "$PROXY" ] || [ "$1" = off ] || return 0
    networksetup -listallnetworkservices | tail -n +2 | grep -v '^\*' | while read -r s; do
        if [ "$1" = on ]; then
            local hp=${PROXY#*://}
            sudo networksetup -setsocksfirewallproxy "$s" "${hp%:*}" "${hp##*:}" 2>/dev/null || true
        else
            sudo networksetup -setsocksfirewallproxystate "$s" off 2>/dev/null || true
        fi
    done
}
clt_install() {
    local label
    softwareupdate --list > "$LOGDIR/softwareupdate-list.txt" 2>&1 || true
    cat "$LOGDIR/softwareupdate-list.txt"
    label=$(sed -n "s/^\* Label: \(Command Line Tools for Xcode-$CLT_VERSION\)\$/\1/p" "$LOGDIR/softwareupdate-list.txt" | head -1)
    # Apple offers the CLT for the macOS it runs on and a few older ones: 15.3 on macOS 14, probably
    # not on macOS 15 or later (untested). Without it, see INSTALL_MACOS.md section 1 (the .dmg
    # from developer.apple.com).
    [ -n "$label" ] || { echo "softwareupdate does not offer Command Line Tools $CLT_VERSION on macOS $(sw_vers -productVersion)"; return 1; }
    sudo softwareupdate --install "$label" --verbose || return 1
}
step_clt() {
    local have p sdks
    have=$(pkgutil --pkg-info=com.apple.pkg.CLTools_Executables 2>/dev/null | awk '/^version:/ {print $2}')
    case "$have" in
        "$CLT_VERSION".*) echo "Command Line Tools $have already installed" ;;
        *)
            if [ -n "$have" ] || [ -d /Library/Developer/CommandLineTools ]; then
                echo "removing Command Line Tools $have"
                sudo rm -rf /Library/Developer/CommandLineTools
                for p in $(pkgutil --pkgs | grep '^com\.apple\.pkg\.CLTools_'); do sudo pkgutil --forget "$p"; done
            fi
            system_proxy on
            sudo touch "$CLT_FLAG"
            local rc=0
            clt_install || rc=$?
            system_proxy off
            sudo rm -f "$CLT_FLAG"
            [ "$rc" = 0 ] || return "$rc"
            ;;
    esac
    system_proxy off   # also after an interrupted install
    sudo xcode-select -s /Library/Developer/CommandLineTools
    xcode-select -p
    pkgutil --pkg-info=com.apple.pkg.CLTools_Executables
    # the default SDK is the target macOS's, and there is none for a newer macOS (CLT 15.3 also
    # has 13.3)
    local major=${macos_target%%.*} v
    sdks=$(ls /Library/Developer/CommandLineTools/SDKs)
    echo "SDKs: $sdks"
    for v in $(echo "$sdks" | sed -n 's/^MacOSX\([0-9][0-9]*\).*\.sdk$/\1/p'); do
        [ "$v" -le "$major" ] || { echo "SDK for macOS $v, newer than $major, found"; return 1; }
    done
    [ "$(xcrun --show-sdk-version | cut -d. -f1)" = "$major" ] || { echo "the default SDK is not macOS $major's"; return 1; }
    xcrun --show-sdk-path
    clang --version
    # Apple clang and its libc++ for C++17 (Qt5 builds) and C++20 (Qt6 builds), for $macos_target
    cat > /tmp/clt-cxx17.cpp <<'EOF'
#include <optional>
#include <string_view>
#include <variant>
#include <filesystem>
#include <shared_mutex>
int main() {
    std::optional<int> o = 1; std::variant<int, double> v = 2.0;
    if constexpr (sizeof(int) == 4) { auto [a, b] = std::pair<int, int>(1, 2); (void)a; (void)b; }
    std::shared_mutex m; std::string_view sv = "x";
    return (*o + (int)std::get<double>(v) + (int)sv.size() + (int)std::filesystem::path("/tmp").empty()) == 4 ? 0 : 1;
}
EOF
    cat > /tmp/clt-cxx20.cpp <<'EOF'
#include <algorithm>
#include <compare>
#include <concepts>
#include <numbers>
#include <ranges>
#include <span>
#include <vector>
#include <bit>
template <std::integral T> constexpr T twice(T x) { return 2 * x; }
struct P { int x, y; auto operator<=>(const P&) const = default; };
consteval int c() { return 3; }
int main() {
    std::vector<int> v{3, 1, 2};
    std::ranges::sort(v);
    std::span<int> s(v);
    auto even = s | std::views::filter([](int i) { return i % 2 == 0; });
    P a{.x = 1, .y = 2}, b{1, 3};
    return (twice(s[0]) == 2 && *even.begin() == 2 && a < b && c() == 3 && std::popcount(7u) == 3 &&
            std::numbers::pi > 3) ? 0 : 1;
}
EOF
    clang++ -std=c++17 -mmacosx-version-min="$macos_target" -O2 /tmp/clt-cxx17.cpp -o /tmp/clt-cxx17 && /tmp/clt-cxx17
    clang++ -std=c++20 -mmacosx-version-min="$macos_target" -O2 /tmp/clt-cxx20.cpp -o /tmp/clt-cxx20 && /tmp/clt-cxx20
    # informational: std::format (libc++ marks it available from macOS 13.3)
    printf '#include <format>\n#include <string>\nint main(){ return std::format("{}", 42) == "42" ? 0 : 1; }\n' > /tmp/clt-format.cpp
    if clang++ -std=c++20 -mmacosx-version-min="$macos_target" /tmp/clt-format.cpp -o /tmp/clt-format && /tmp/clt-format; then
        echo "std::format: OK"
    else
        echo "std::format: not usable"
    fi
    vtool -show-build /tmp/clt-cxx20 | grep -E 'minos|sdk'
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
    # the .pkg's postinstall runs "port selfupdate", whose rsync can't use the SOCKS proxy and hangs
    # for minutes on a guest without direct internet access: stop it (ports_https_source syncs later)
    if [ -n "$PROXY" ]; then
        ( while sleep 15; do
              pgrep -qx installer || break
              sudo pkill -f 'rsync://rsync\.macports\.org' 2> /dev/null || true
          done ) &
    fi
    sudo installer -pkg "$dl/$pkg" -target /
    wait
    port version
}

# Build every port for $macos_target in macports.conf. On macOS $macos_target itself that is
# MacPorts' default, and its binary archives are built for it; on a newer macOS, ports are built
# from source, since the archives would need that macOS. This applies to the ports installed
# afterwards; step_rebuild_target rebuilds the installed ones that are too new.
# macosx_sdk_version keeps MacPorts' default (this OS's SDK, the CLT's macOS 14 SDK in the tart image).
# Never marked done: a MacPorts upgrade (step_macports) must not lose the settings.
set_macports_conf() { # key value
    local conf=/opt/local/etc/macports/macports.conf
    if grep -q "^$1[[:space:]]" $conf; then
        sudo perl -pi -e "s|^$1\\s.*|$1\\t$2|" $conf
    else
        printf '# Natron (tools/tart/provision.sh)\n%s\t%s\n' "$1" "$2" | sudo tee -a $conf > /dev/null
    fi
}
step_macports_conf() {
    set_macports_conf macosx_deployment_target "$macos_target"
    if [ "$(sw_vers -productVersion | cut -d. -f1)" = "${macos_target%%.*}" ]; then
        set_macports_conf buildfromsource ifneeded
    else
        # UNTESTED: an SDK image on a macOS newer than $macos_target (BASE_IMAGE). Every port is
        # then built from source, and qt5-qtwebengine needs a full Xcode (see INSTALL_MACOS.md).
        set_macports_conf buildfromsource always
    fi
    grep -E '^(macosx_deployment_target|macosx_sdk_version|buildfromsource)[[:space:]]' /opt/local/etc/macports/macports.conf
    grep -q "^macosx_deployment_target[[:space:]]*$macos_target\$" /opt/local/etc/macports/macports.conf
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
    (cd "$NATRON/tools/MacPorts" && { ./check.sh --all || true; }) | tee "$LOGDIR/check.txt"
    if grep -q "is obsolete\|is missing\|differs from upstream\|is unused\|has no Portfile" "$LOGDIR/check.txt"; then
        echo "WARNING: overlay is out of date with this upstream tree (see $LOGDIR/check.txt)"
    fi
    sudo mkdir -p /opt/MacPorts-Natron
    sudo chown "$(id -un):staff" /opt/MacPorts-Natron
    grep -q "^file:///opt/MacPorts-Natron" /opt/local/etc/macports/sources.conf ||
        sudo perl -0pi -e 's|^(\S+\s+\[default\])|file:///opt/MacPorts-Natron\n$1|m' /opt/local/etc/macports/sources.conf
    step_sync
    port file openimageio | grep -q '^/opt/MacPorts-Natron/' || { echo "overlay not used"; exit 1; }
}

# OSMesa and LLVM are static libraries linked into Natron: build them for $macos_target too
# (osmesa-install.sh honours MACOSX_DEPLOYMENT_TARGET). Rebuilt if built for a newer macOS.
osmesa_ok() {
    [ -f /opt/osmesa/lib/libMangledOSMesa32.a ] && [ -x /opt/llvm/bin/llvm-config ] &&
        "$CHECK_MINOS" -N -t "$macos_target" /opt/osmesa/lib /opt/llvm/lib/libLLVMSupport.a > /dev/null
}
step_osmesa() {
    if osmesa_ok; then return; fi
    if [ -d /opt/osmesa/lib ] || [ -d /opt/llvm/lib ]; then
        echo "OSMesa/LLVM missing or built for a macOS newer than $macos_target: rebuilding them"
        sudo rm -rf /opt/osmesa /opt/llvm
    fi
    sudo mkdir -p /opt/osmesa /opt/llvm
    sudo chown "$(id -un):staff" /opt/osmesa /opt/llvm
    $PORT install python27 cmake autoconf automake libtool pkgconfig gsed
    sudo port select --set python python27
    mkdir -p "$OSMESA_SRC/build"
    (cd "$OSMESA_SRC/build" && env LLVM_BUILD=1 MKJOBS="$JOBS" MACOSX_DEPLOYMENT_TARGET="$macos_target" ../osmesa-install.sh)
    sudo port select --set python none
    osmesa_ok
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
    $PORT install seexpr211
    $PORT install librsvg -quartz
    $PORT install poppler librevenge libcdr-0.1 sox libzip
    sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
    $PORT install ImageMagick +natron -x11
    $PORT install ImageMagick7 +natron -x11 -openexr
    $PORT install ffmpeg +gpl2 +highbitdepth +natronmini
    # COMMERCIAL builds (NATRON_LICENSE=COMMERCIAL): LGPL FFmpeg, LibRaw and libheif in libexec
    $PORT install ffmpeg-lgpl libraw-lgpl libheif-lgpl
    $PORT install openimageio +freetype +natron +opencolorio +openjpeg $(oiio_python_variants)
}

# pip can't use a SOCKS proxy (vendored urllib3 bug), so fetch the wheel with curl,
# check it against the expected sha256, and install it offline.
pip_install_wheel() { # project version sha256
    local json url file sha d
    d=$(mktemp -d /tmp/wheel.XXXX)
    json=$(curl -fsSL "https://pypi.org/pypi/$1/$2/json")
    read -r url sha < <(printf '%s' "$json" | python$pyver -c 'import json,sys
u=[u for u in json.load(sys.stdin)["urls"] if u["packagetype"]=="bdist_wheel"][0]
print(u["url"], u["digests"]["sha256"])')
    file=$d/$(basename "$url")
    curl -fsSL -o "$file" "$url"
    [ "$sha" = "$3" ] || { echo "PyPI lists sha256 $sha for $1 $2, expected $3"; return 1; }
    [ "$(shasum -a 256 "$file" | cut -d" " -f1)" = "$3" ] || { echo "sha256 mismatch for $file"; return 1; }
    sudo pip-$pyver install --no-index --no-deps "$file"
    python$pyver -c "import importlib.metadata as m; assert m.version('$1') == '$2'"
}

step_python_qt() {
    $PORT install py$pyv-pip
    $PORT install py$pyv-numpy -gfortran
    $PORT install py$pyv-sphinx py$pyv-sphinx_rtd_theme
    $PORT install qt5 qt5-qtwebengine
    $PORT install py$pyv-pyside2
    sudo port select --set python python$pyv
    sudo port select --set python3 python$pyv
    sudo port select --set sphinx py$pyv-sphinx
    pip_install_wheel QtPy 1.11.2 83c502973e9fdd7b648d8267a421229ea3d9a0651c22e4c65a4d9228479c39b6
    local py=/opt/local/Library/Frameworks/Python.framework/Versions/$pyver
    [ -e $py/lib/python$pyver/site-packages/shiboken2_generator/shiboken2-$pyver ] ||
        sudo ln -s shiboken2 $py/lib/python$pyver/site-packages/shiboken2_generator/shiboken2-$pyver
    sudo ln -sf shiboken2-$pyver /opt/local/bin/shiboken2
    [ -e /opt/local/Library/lib ] || sudo ln -s Frameworks /opt/local/Library/lib
    [ -f $py/lib/python$pyver/site-packages/shiboken2_generator/shiboken2-$pyver ]
}

# Idempotent fixes of installed files that a port upgrade may undo, and the port selections
# (step_osmesa temporarily selects python27). Never marked done: re-run on every create/update.
step_fixups() {
    system_proxy off   # only step_clt turns it on, for softwareupdate
    [ -e /opt/local/include/minizip-ng ] || sudo ln -s . /opt/local/include/minizip-ng
    sudo sed -i '' -e 's/^find_dependency(zstd/#find_dependency(zstd/' /opt/local/lib/cmake/minizip-ng/minizip-ng-config.cmake
    sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
    sudo port select --set python python$pyv
    sudo port select --set python3 python$pyv
    sudo port select --set sphinx py$pyv-sphinx
    local py=/opt/local/Library/Frameworks/Python.framework/Versions/$pyver
    [ -e $py/lib/python$pyver/site-packages/shiboken2_generator/shiboken2-$pyver ] ||
        sudo ln -s shiboken2 $py/lib/python$pyver/site-packages/shiboken2_generator/shiboken2-$pyver
    sudo ln -sf shiboken2-$pyver /opt/local/bin/shiboken2
    [ -e /opt/local/Library/lib ] || sudo ln -s Frameworks /opt/local/Library/lib
    if [ -x /opt/local/bin/dmgbuild-$pyver ]; then sudo ln -sf dmgbuild-$pyver /opt/local/bin/dmgbuild; fi
}

step_dmgtools() {
    $PORT install py$pyv-dmgbuild
    # the port only installs dmgbuild-$pyver (no port select group)
    [ -e /opt/local/bin/dmgbuild ] || sudo ln -s dmgbuild-$pyver /opt/local/bin/dmgbuild
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
    # an empty or partial overlay also gives "Ports failed: 0": check that MacPorts really uses it
    port file openimageio | grep -q '^/opt/MacPorts-Natron/' || { echo "overlay not used (port file openimageio)"; return 1; }
}

# Ports pinned in the overlay: the port directories with a PINNED file
# (tools/MacPorts/README.txt), pinned either ahead of upstream (a newer version)
# or held at an older one. MacPorts never downgrades, and an "ahead" port may
# have been installed from upstream before, so reinstall a pinned port whenever
# its installed version differs from the overlay's.
pinned_ports() {
    local f d n
    for f in "$NATRON"/tools/MacPorts/*/*/PINNED; do
        [ -f "$f" ] || continue
        d=$(dirname "$f")
        n=$(awk '$1 == "name" {print $2; exit}' "$d/Portfile" 2>/dev/null)
        case "$n" in ''|*'$'*|*'['*) n=$(basename "$d") ;; esac
        echo "$n"
    done
}
step_pins() {
    local p v want have variants vfile
    for p in $(pinned_ports); do
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
                # one version at a time: with several installed, "port uninstall $p" refuses to choose
                for v in $(port -q installed "$p" | awk '{print $2}'); do
                    $SUDO port -v -N -f uninstall "$p" "$v"
                done
                $PORT install "$p" $variants
                ;;
        esac
        rm -f "$vfile"
    done
}

# An SDK provisioned with an older Python (3.10 until 2026-11-17): install the py$pyv ports
# and switch openimageio's python variant. Each check looks at the result of the step, not at
# a marker, so that an interrupted migration is completed by the next update (python_qt and
# dmgtools are idempotent). The fixups step, run after update, then selects python$pyv and
# re-creates the links. The old pyNNN ports are left installed (see INSTALL_MACOS.md to remove them).
step_python_migrate() {
    local site=/opt/local/Library/Frameworks/Python.framework/Versions/$pyver/lib/python$pyver/site-packages
    if ! port -q installed py$pyv-pyside2 2>/dev/null | grep -q '(active)' ||
        ! [ -f $site/shiboken2_generator/shiboken2-$pyver ] ||
        ! python$pyver -c 'import qtpy, PySide2.QtCore' 2>/dev/null; then
        echo "===== installing the Python $pyver ports"
        step_python_qt
    fi
    if ! [ -x /opt/local/bin/dmgbuild-$pyver ]; then
        echo "===== installing dmgbuild for Python $pyver"
        step_dmgtools
    fi
    if ! port -q installed openimageio 2>/dev/null | grep '(active)' | grep -q "+python$pyv"; then
        echo "===== switching openimageio to +python$pyv"
        $PORT -n upgrade --enforce-variants openimageio +freetype +natron +opencolorio +openjpeg $(oiio_python_variants)
    fi
}

# The active ports that have a Mach-O file built for a macOS newer than $macos_target: installed
# from MacPorts' binary archives, or built before step_macports_conf set the deployment target.
ports_newer_than_target() {
    local p rc
    for p in $(port -q installed active | awk '{print $1}'); do
        rc=0
        # -N: ports may install files without a minimum version (e.g. archives of non-Mach-O
        # objects); the DMG check (natron-build.sh, launchBuildMain.sh) is strict
        port -q contents "$p" | "$CHECK_MINOS" -N -t "$macos_target" -l > /dev/null 2>&1 || rc=$?
        case $rc in
            0) ;;
            1) echo "$p" ;;
            *) echo "WARNING: check-minos.sh failed (status $rc, otool could not read a file) on the files of $p" >&2 ;;
        esac
    done
}

# Print the given ports in dependency order (dependencies first; tsort(1) breaks cycles).
deps_order() {
    local set=" $* " p rest d
    {
        for p in "$@"; do echo "$p $p"; done
        port -q info --line --name --depends_fetch --depends_extract --depends_patch \
            --depends_build --depends_lib --depends_run "$@" |
            while IFS=$'\t' read -r p rest; do
                for d in $(echo "$rest" | tr ',' ' '); do
                    case "$set" in *" $d "*) [ "$d" = "$p" ] || echo "$d $p" ;; esac
                done
            done
    } | tsort 2>/dev/null
}

# Rebuild from source, for $macos_target, every installed port built for a newer macOS, in
# dependency order (so that static libraries and headers come from rebuilt dependencies).
# Resumable: a port is selected by its files, so the ports already rebuilt are skipped.
# A port still too new after its rebuild (prebuilt binaries, or a build system ignoring
# MACOSX_DEPLOYMENT_TARGET) is listed in rebuild_target.skip with its version, and only rebuilt
# again when its version changes; the minos check of the DMG (check-minos.sh, natron-build.sh)
# tells whether that matters. Never marked done: run after every update.
step_rebuild_target() {
    local skip=$LOGDIR/rebuild_target.skip p v todo=() failed=() n=0
    touch "$skip"
    local newer
    newer=$(ports_newer_than_target)   # not in the for: a failure must stop the step
    for p in $newer; do
        v=$(port -q installed "$p" | awk '/\(active\)/ {print $2}')
        if grep -qxF "$p $v" "$skip"; then
            echo "$p $v: still built for a macOS newer than $macos_target after a rebuild, skipped"
            continue
        fi
        todo+=("$p")
    done
    [ ${#todo[@]} -gt 0 ] || { echo "all installed ports are built for macOS $macos_target or older"; return 0; }
    echo "===== ${#todo[@]} ports built for a macOS newer than $macos_target: ${todo[*]}"
    for p in $(deps_order "${todo[@]}"); do
        n=$((n + 1))
        echo "===== [$n/${#todo[@]}] rebuilding $p for macOS $macos_target $(date '+%F %T')"
        if ! $PORT -n -s upgrade --force "$p"; then
            # the previous build stays installed: go on with the other ports, fail at the end
            echo "ERROR: could not rebuild $p"
            failed+=("$p")
            continue
        fi
        if port -q contents "$p" | "$CHECK_MINOS" -N -t "$macos_target" -l; then :; else
            v=$(port -q installed "$p" | awk '/\(active\)/ {print $2}')
            echo "WARNING: $p $v is still built for a macOS newer than $macos_target"
            echo "$p $v" >> "$skip"
        fi
    done
    echo "===== ports still built for a macOS newer than $macos_target (see $skip):"
    cat "$skip"
    if [ ${#failed[@]} -gt 0 ]; then echo "===== ports that could not be rebuilt: ${failed[*]}"; return 1; fi
}

# Update an existing SDK image: refresh MacPorts and the overlay, upgrade outdated ports.
step_update() {
    # resumable: "natron-sdk.sh create" re-runs the update while this marker exists
    date '+%F %T' > "$LOGDIR/update.pending"
    # MacPorts base: install the .pkg of MACPORTS_VERSION if it differs from the installed one
    step_macports
    # build for $macos_target, from source (also after a MacPorts upgrade)
    step_macports_conf
    ports_https_source
    (cd "$NATRON/tools/MacPorts" && { ./check.sh --all || true; }) | tee "$LOGDIR/check.txt"
    if grep -q "is obsolete\|is missing\|differs from upstream\|is unused\|has no Portfile" "$LOGDIR/check.txt"; then
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
        # uninstall every installed version (e.g. an inactive 8.1.2 next to the active 9.x):
        # "port uninstall ffmpeg" refuses to choose when there are several
        for v in $(port -q installed ffmpeg 2>/dev/null | awk '{print $2}'); do
            $PORT -f uninstall ffmpeg "$v"
        done
        $PORT install ffmpeg +gpl2 +highbitdepth +natronmini
        rm -f "$LOGDIR/ffmpeg-novmaf.pending"
        # libvmaf is no longer needed by anything
        $PORT -N uninstall libvmaf 2>/dev/null || true
    fi
    # COMMERCIAL builds: LGPL FFmpeg, LibRaw and libheif (images made before these subports)
    for p in ffmpeg-lgpl libraw-lgpl libheif-lgpl; do
        if ! port -q installed "$p" 2>/dev/null | grep -q '(active)'; then
            echo "===== installing $p"
            $PORT install "$p"
        fi
    done
    step_python_migrate
    # ImageMagick 7 for openfx-arena (Natron 2.6), for the images created before step_libs had it
    if ! port -q installed ImageMagick7 2>/dev/null | grep -q '+natron.*(active)'; then
        echo "===== installing ImageMagick7 +natron"
        $PORT install ImageMagick7 +natron -x11 -openexr
    fi
    # rebuild OSMesa/LLVM if missing or built for a newer macOS (delete /opt/osmesa/lib to force a rebuild)
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
    if [ "${NATRON_LICENSE:-GPL}" = "COMMERCIAL" ]; then
        # step_libs and step_update install them: fail now rather than after building Natron
        for p in ffmpeg-lgpl libraw-lgpl libheif-lgpl; do
            if ! port -q installed "$p" 2>/dev/null | grep -q '(active)'; then
                echo "Error: $p is not installed in this image, update it with: tools/tart/natron-sdk.sh update"
                return 1
            fi
        done
    fi
    mkdir -p "$WORKSPACE"
    cd "$NATRON/tools/jenkins"
    env NOUPDATE=1 WORKSPACE="$WORKSPACE" DISABLE_BREAKPAD=1 NATRON_MACOS_MIN="$macos_target" NATRON_LICENSE="${NATRON_LICENSE:-GPL}" \
        GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH="${GIT_BRANCH:-RB-2.6}" \
        UNIT_TESTS="${UNIT_TESTS:-false}" BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 \
        COMPILE_TYPE=release MKJOBS="$JOBS" \
        NATRON_BUILD_SYSTEM="${NATRON_BUILD_SYSTEM:-cmake}" NATRON_EXTRA_CMAKE_FLAGS="${NATRON_EXTRA_CMAKE_FLAGS:-}" \
        ./launchBuildMain.sh
    # launchBuildMain.sh can exit 0 after a failed sub-step: require the DMG
    ls -l "$WORKSPACE/builds_archive/natron_github_RB2/1/"
    ls "$WORKSPACE"/builds_archive/natron_github_RB2/1/*.dmg
}

SDK_STEPS="disk nobrew clt macports macports_conf overlay sync pins osmesa compiler tools libs python_qt dmgtools rebuild_target fixups"
# steps that are never marked done (they are meant to be re-run)
NO_MARKER_STEPS="disk macports_conf sync pins rebuild_target update fixups cleanup build"
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
