Instructions for installing Natron from sources on macOS
=======================================================

These are step-by-step instructions to compile Natron on macOS. This file is the reference for macOS installs. There are two ways to build:

- **In a VM, fully automated:** `tools/tart/README.md` builds the same MacPorts SDK in a clean macOS 14 (Sonoma) [tart](https://tart.run) VM, starting from the stock `ghcr.io/cirruslabs/macos-sonoma-base` image (see [0. A clean macOS 14 VM](#0-a-clean-macos-14-vm-tart)), builds the `.dmg` in a throw-away clone, and runs the regression tests on the `.dmg` in a bare clone of the base image. Its provisioning script (`tools/tart/provision.sh`) runs the MacPorts steps below. tart only runs on **Apple Silicon Macs** and only produces arm64 builds; for Intel or universal builds, or for macOS 11 and older, use the MacPorts steps below on an Intel Mac (see "Notes for older systems"). On any Mac, the MacPorts steps below can also be run natively.
- **On your Mac:** follow the [MacPorts section](#macports-from-scratch) below.

The official macOS binaries are built with [MacPorts](https://www.macports.org) plus the custom ports in `tools/MacPorts`, using the `tools/jenkins/launchBuildMain.sh` script. That script does everything: checks out the sources, compiles the plugins and Natron, builds the `.dmg`, runs the tests and archives the result. The [MacPorts section](#macports-from-scratch) below describes how to prepare a Mac for it from scratch. On Apple Silicon it produces native arm64 binaries.

Homebrew is not supported: build with MacPorts only, and keep Homebrew out of the `PATH` (see `port_switch` below). A Homebrew tool or library found by a build silently replaces a missing MacPorts one.

Natron is built with **CMake** (see [Building with CMake](#building-with-cmake)). The release scripts (`launchBuildMain.sh`, and `tools/tart/provision.sh`, which drives it) build Natron with CMake too. The qmake build is deprecated: on RB-2.6 it stays available to them as a fallback (`NATRON_BUILD_SYSTEM=qmake`) until the CMake-built installers are shown to be equivalent (#1181); it will be removed in a future release.

The workflow around the release build is documented in the Maintainer Guide (`Documentation/source/maintainers/build-macos.rst`). That covers keeping MacPorts up to date, the overlay maintenance, signing, and the test suite.

## Before you start

**macOS 14 (Sonoma) is the distribution baseline** of Natron 2.6 (maintainer decision, 2026-11-21): the release is built on macOS 14, for macOS 14.0 and later, with only the Xcode Command Line Tools and their macOS 14 SDK (no Xcode). GitHub's CI uses `macos-15` runners, only because GitHub has no macOS 14 runners. The steps below also work on a newer macOS, with the extra settings of section 2.

A from-scratch MacPorts install builds LLVM, Mesa (OSMesa), Qt 5's patched `qtbase` and a few ports with Natron variants from source, and installs the other ports from MacPorts' binary archives: plan for several hours and about 60 GB of free disk space. Run the long steps inside `caffeinate -i` or `tmux`.

## Checkout sources

The MacPorts overlay is part of the source tree, so check out the sources first:

```Shell
git clone https://github.com/NatronGitHub/Natron.git
cd Natron
git submodule update --init --recursive
```

`--recursive` matters: `libs/SequenceParsing` has its own submodule (`tinydir`), which the build needs.

In the rest of this document, `$NATRON_SRC` is the absolute path to this checkout. Set it in every shell you build from:

```Shell
export NATRON_SRC=$PWD   # in the Natron directory
export CLANG=17          # the MacPorts clang of step 5: 17 on macOS 14, the baseline (see the table there)
```

## Install Dependencies

Natron is built with MacPorts (as the official builds). Homebrew is not supported.

### MacPorts from scratch

Run every step below from a terminal. Some `port install` steps take a long time (clang and Qt are built from source when no binary archive is available), so running them inside `caffeinate -i` (or `tmux`, installed in step 6) is a good idea.

If Homebrew is also installed, keep it out of your `PATH` while building with MacPorts. Otherwise build scripts may silently pick up Homebrew tools and hide missing MacPorts dependencies. These shell functions (bash or zsh: put them in `~/.zshrc`, or `~/.bash_profile` for bash) switch between the two:

```Shell
path_remove() {  # remove one exact entry from PATH
  PATH=":$PATH:"; PATH=${PATH//":$1:"/":"}; PATH=${PATH#:}; PATH=${PATH%:}
}
port_switch() {  # use MacPorts, hide Homebrew
  local p
  for p in /opt/homebrew /usr/local; do
    if [ -x "$p/bin/brew" ]; then
      path_remove "$p/bin"; path_remove "$p/sbin"; path_remove "$p/opt/python/libexec/bin"
    fi
  done
  unset HOMEBREW_PREFIX HOMEBREW_CELLAR HOMEBREW_REPOSITORY
  path_remove /opt/local/bin; path_remove /opt/local/sbin
  if [ -x /opt/local/bin/port ]; then PATH="/opt/local/bin:/opt/local/sbin:$PATH"; fi
}
brew_switch() {  # use Homebrew, hide MacPorts
  path_remove /opt/local/bin; path_remove /opt/local/sbin
  if [ -x /opt/homebrew/bin/brew ]; then eval "$(/opt/homebrew/bin/brew shellenv)"
  elif [ -x /usr/local/bin/brew ]; then eval "$(/usr/local/bin/brew shellenv)"; fi
}
```

Run `port_switch` in the terminal you build from (before `launchBuildMain.sh` or `make`).

#### 0. A clean macOS 14 VM (tart)

The release SDK is installed in a [tart](https://tart.run) VM made from the stock cirruslabs Sonoma image; `tools/tart/natron-sdk.sh create` does all of this section and the next ones (see `tools/tart/README.md`). To do it by hand, on an **Apple Silicon** host (tart only runs there, and only arm64 guests):

```Shell
# the image used for the release SDK (NATRON_MACOS_BASE_IMAGE in tools/jenkins/macos-release.sh),
# pinned by digest; ghcr.io/cirruslabs/macos-sonoma-base:latest is the newest one
IMAGE=ghcr.io/cirruslabs/macos-sonoma-base@sha256:e2ebdfc4d354b336fe00d729c11a8136a019b8046f41cc183a2fe51b85d18f49
tart pull $IMAGE                                               # about 30 GB to download
tart clone $IMAGE natron-sdk-sonoma
tart set natron-sdk-sonoma --cpu 6 --memory 16384 --disk-size 250   # GB; the disk is sparse
tart run --no-graphics natron-sdk-sonoma &
ssh admin@$(tart ip natron-sdk-sonoma)                          # password: admin
```

The image (macOS 14.8.7 in 2026-11) has a single account, `admin` with password `admin` and passwordless `sudo`, which logs in automatically. On its first boot macOS grows the APFS container to the new disk size (`sudo diskutil apfs resizeContainer disk0s2 0` does it otherwise). (The scripts use `NATRON_MACOS_BASE_IMAGE` from `tools/jenkins/macos-release.sh`, pinned by digest for reproducible release images; `BASE_IMAGE=ghcr.io/cirruslabs/macos-sonoma-base:latest` overrides it, to try a newer image before moving the pin.)

If the guest gets no IPv4 address, or no internet access (seen behind corporate network filters, see "Networking" in `tools/tart/README.md`), reach it at its IPv6 link-local address, and give it internet access through a SOCKS proxy carried by the SSH connection, as the scripts do:

```Shell
# on the host: the guest's link-local address, on the vmnet bridge (bridge100, the one with 192.168.64.1)
A=$(tart exec natron-sdk-sonoma /sbin/ifconfig en0 | awk '/inet6 fe80/{sub(/%.*/,"",$2); print $2; exit}')
# a SOCKS proxy on the guest's 127.0.0.1:1080 only, to ports 80 and 443 only (OpenSSH 8.7 or later)
ssh -R 127.0.0.1:1080 -o PermitRemoteOpen='*:80 *:443' admin@$A%bridge100
# in the guest (curl, git and MacPorts honour it; with sudo, pass it: sudo env ALL_PROXY=... port ...)
export ALL_PROXY=socks5h://127.0.0.1:1080
```

**Order of the steps in the VM.** This section and the next one replace the Command Line Tools, so `git` doesn't work in between: in the VM, check out the sources (see "Checkout sources") after section 1, or copy them from the host (`natron-sdk.sh` copies the files that git tracks under `tools/` with rsync).

The commands of this section and of section 1 remove Homebrew and the Command Line Tools and change system settings: **run them only inside the VM**, never on your own Mac. `tools/tart/provision.sh`, which runs them for `natron-sdk.sh`, refuses to run outside a tart VM.

The image comes with things a Natron SDK must not use:

- **Homebrew** in `/opt/homebrew` (cmake, pkgconf, python@3.14, gcc, node, …), added to the `PATH` by `/etc/paths.d/homebrew` and by `eval "$(/opt/homebrew/bin/brew shellenv)"` in `~/.zprofile` (`~/.profile` is a link to it). Remove it. The tart guest agent (needed by `tart exec`) was installed with Homebrew, so move it out first:

  ```Shell
  sudo mkdir -p /usr/local/libexec
  sudo cp "$(readlink -f /opt/homebrew/bin/tart-guest-agent)" /usr/local/libexec/tart-guest-agent
  sudo perl -pi -e 's|/opt/homebrew/bin/tart-guest-agent|/usr/local/libexec/tart-guest-agent|; s|:/opt/homebrew/bin||' \
      /Library/LaunchAgents/org.cirruslabs.tart-guest-agent.plist /Library/LaunchDaemons/org.cirruslabs.tart-guest-daemon.plist
  sudo rm -rf /opt/homebrew /etc/paths.d/homebrew ~/.homebrew
  grep -v -E 'brew|HOMEBREW|rbenv|node@|PNPM' ~/.zprofile > ~/.zprofile.new && mv ~/.zprofile.new ~/.zprofile
  git config --global --remove-section filter.lfs   # git-lfs came from Homebrew
  ```

- **Command Line Tools 16.2**, with the macOS 15.2 SDK as the default SDK and Apple clang 16. They are replaced by the Command Line Tools 15.3 in the next section.

#### 1. Command Line Tools with the macOS 14 SDK

Natron needs only the **Command Line Tools for Xcode 15.3**: their default SDK is macOS 14.4's (`MacOSX.sdk` → `MacOSX14.4.sdk`; there is also a `MacOSX13.3.sdk`), and their compiler is Apple clang 15.0.0 (`clang-1500.3.9.4`), which builds Natron's C++17 (Qt 5) and C++20 (Qt 6) code for macOS 14.0. Xcode is not needed. (The one port that requires Xcode to build, `qt5-qtwebengine`, which `py311-pyside2` depends on, is installed from MacPorts' binary archive.) Newer Command Line Tools (16.x) default to a macOS 15 SDK: don't use them for the release.

`softwareupdate` installs the Command Line Tools without an Apple ID when a marker file exists. First remove the installed ones if they are another version (`pkgutil --pkg-info=com.apple.pkg.CLTools_Executables` shows it):

```Shell
sudo rm -rf /Library/Developer/CommandLineTools
for p in $(pkgutil --pkgs | grep '^com\.apple\.pkg\.CLTools_'); do sudo pkgutil --forget $p; done
sudo touch /tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
softwareupdate --list                     # lists "Command Line Tools for Xcode-15.3" (and 16.2) on macOS 14
sudo softwareupdate --install "Command Line Tools for Xcode-15.3" --verbose   # about 700 MB
sudo rm /tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress
sudo xcode-select -s /Library/Developer/CommandLineTools
xcrun --show-sdk-version                  # 14.4
clang --version                           # Apple clang version 15.0.0 (clang-1500.3.9.4)
```

`softwareupdate` doesn't use `ALL_PROXY`: in a VM whose internet access goes through the SSH SOCKS proxy, set the system proxy before `softwareupdate --list`, on every network service, and turn it off afterwards:

```Shell
services() { networksetup -listallnetworkservices | tail -n +2 | grep -v '^\*'; }
services | while read -r s; do sudo networksetup -setsocksfirewallproxy "$s" 127.0.0.1 1080; done
# ... softwareupdate --list and --install, as above ...
services | while read -r s; do sudo networksetup -setsocksfirewallproxystate "$s" off; done
```

The `clt` step of `tools/tart/provision.sh` does this inside the VM, and checks that the default SDK is macOS 14's and that small C++17 and C++20 programs (concepts, ranges, `std::span`, `<=>`, `std::format`) build for macOS 14.0. (On a Mac of your own running macOS 14, `xcode-select --install` installs the newest Command Line Tools offered, 16.x, which is fine for development builds.)

If Apple stops offering the Command Line Tools 15.3 through `softwareupdate`, download "Command Line Tools for Xcode 15.3" from <https://developer.apple.com/download/all/> (this needs an Apple ID), copy the `.dmg` into the VM, and install it: `hdiutil attach Command_Line_Tools_for_Xcode_15.3.dmg`, then `sudo installer -pkg "/Volumes/Command Line Developer Tools/Command Line Tools.pkg" -target /`. The version is set by `NATRON_MACOS_CLT_VERSION` in `tools/jenkins/macos-release.sh`.

The release scripts build Natron and the plug-ins with this Apple clang (`tools/jenkins/compiler-common.sh`, #1134), as long as it supports the C++ standard of the build (C++17 with Qt5, C++20 with Qt6) and links with MacPorts' `libomp` (section 5): Natron's `CMakeLists.txt` and the plug-ins (`OFX_LIBOMP_ROOT`, passed by `build-plugins.sh`) find it, as in "Developing with the MacPorts SDK". Apple clang's driver has no `-fopenmp` (only `-Xpreprocessor -fopenmp`) and Xcode ships no OpenMP runtime, so the builds that pass `-fopenmp` to the compiler, the deprecated qmake build (`NATRON_BUILD_SYSTEM=qmake`, `CONFIG+=openmp`) and the plug-ins' Makefiles (`PLUGINS_BUILD_SYSTEM=make`, `OPENMP=1`), use a MacPorts clang instead (section 5), as does `NATRON_MACOS_CLANG=macports`. Apple clang also builds the ports whose Portfile selects it.

On a Mac (not needed in the VM), under *System Settings › Privacy & Security › Developer Tools* (*System Preferences › Security & Privacy › Privacy › Developer Tools* before macOS 13), enable your terminal application. This avoids Gatekeeper scans on every freshly built binary.

#### 2. MacPorts

Download and install the `.pkg` for your macOS version from <https://www.macports.org/install.php> (or <https://github.com/macports/macports-base/releases/latest>). Then update it:

```Shell
sudo port selfupdate
```

**In a VM without direct internet access:** the `.pkg`'s postinstall runs `port selfupdate`, whose rsync can't use the SOCKS proxy and hangs until it times out (several minutes); stop it from another ssh session with `sudo pkill -f rsync://rsync.macports.org`, or wait. Then fetch the ports tree over HTTPS, through the proxy, instead of `selfupdate`:

```Shell
sudo perl -pi -e 's|^rsync://\S+(\s+\[default\])|https://distfiles.macports.org/ports.tar.gz$1|' /opt/local/etc/macports/sources.conf
sudo env ALL_PROXY=socks5h://127.0.0.1:1080 port -v sync
```

**Natron 2.6 for macOS runs on macOS 14 (Sonoma) or later**, so the release build, and the MacPorts libraries bundled in the `.dmg`, are built for macOS 14 (minimum version `minos` 14.0). On macOS 14 itself, that is MacPorts' default, and its binary archives (built for the macOS they were made on) can be used. Make it explicit in `/opt/local/etc/macports/macports.conf` **before installing any port**:

```
macosx_deployment_target	14.0
```

MacPorts falls back to building a port from source, silently, when the download of its binary archive fails, which can turn seconds into hours (`llvm-18`), or fail (`qt5-qtwebengine` needs Xcode to build). On a slow or flaky connection, fetch the archives first, then remove the work directories that `archivefetch` leaves (they have the dependencies' default variants, and `install` propagates the requested variants to the dependencies: without the `clean`, `install glib2 -x11` refuses to go on with a `glib2-bootstrap +x11` work directory), then install:

```Shell
sudo port archivefetch <port> <variants> rdepof:<port>
sudo port clean --work <port> <variants> rdepof:<port>   # the fetched archives are kept
sudo port install <port> <variants>
```

`tools/tart/provision.sh` does this, with retries, before every `install` and `upgrade` (except the source builds, `-s`). In a VM without direct internet access, the `.pkg`'s own `port selfupdate` (rsync) hangs until it times out, and the ports tree must be fetched over HTTPS through the proxy (see "Networking" in `tools/tart/README.md`).

On a **newer** macOS, a release SDK also needs `buildfromsource	always`: MacPorts' binary archives would need that macOS (e.g. minos 15.0 on Sequoia), so every port must be built from source, which takes many hours. `qt5-qtwebengine` (a dependency of `py311-pyside2`) then needs a full Xcode to build: install Xcode from the App Store or <https://developer.apple.com/download/all/>, then `sudo xcode-select -s /Applications/Xcode.app/Contents/Developer` and `sudo xcodebuild -license accept`. This path is not tested with the current scripts (the tart image runs macOS 14), and `softwareupdate` probably doesn't offer the Command Line Tools 15.3 there. Leave `macosx_sdk_version` unset: MacPorts then uses the SDK of the running macOS (the Command Line Tools' macOS 14.4 SDK on macOS 14). For a development build that you only run on your own Mac, you can skip this. If ports were installed before these lines were added, rebuild the ones built for a newer macOS (see "Building for an older macOS" below).

#### 3. Natron's custom ports

Natron needs modified ports. Examples: the `+natron` variants of ImageMagick, ImageMagick7 and openimageio and the `+natronmini` variant of ffmpeg; libraw 0.22.3 with the GPL demosaic packs restored by a patch; x265 with the 10- and 12-bit libraries (#1658); the subports `ffmpeg-lgpl`, `libraw-lgpl` and `libheif-lgpl` for COMMERCIAL builds; freetype 2.15.0 with an upstream fix for rotated text backported; onetbb with a patch for a crash at exit (#1253); qt5-qtbase with a patch for a QMutex lost wake-up that hangs render threads on Apple Silicon (#1451); ffmpeg and ffmpeg-lgpl with patches for black 1-pixel columns in 10-bit DNxHD/DNxHR decoded on Apple Silicon ([openfx-io#266](https://github.com/NatronGitHub/openfx-io/issues/266)) and for a DNxHR encoder abort on high-entropy images ([openfx-io#267](https://github.com/NatronGitHub/openfx-io/issues/267)). If you already have a MacPorts SDK with qt5, run `sudo port -v upgrade qt5-qtbase` after `./sync.sh` below: the patched revision (5.15.19_1) has no binary archive, so it builds from source. Likewise, run `sudo port -v upgrade ffmpeg` (and `ffmpeg-lgpl`, if installed) to get the patched FFmpeg (9.0.2_5). Run `sudo port -v upgrade x265` to get the 10- and 12-bit HEVC encoder (4.2_101; `x265 --version` shows `8bit+10bit+12bit`); FFmpeg needs no rebuild. MacPorts reads them from a copy of `tools/MacPorts` in `/opt/MacPorts-Natron`. A copy is used rather than the source tree itself, because MacPorts accesses the ports as an unprivileged user that usually can't read your home directory.

```Shell
sudo mkdir -p /opt/MacPorts-Natron
sudo chown $(id -un):staff /opt/MacPorts-Natron
cd $NATRON_SRC/tools/MacPorts
./sync.sh          # rsync to /opt/MacPorts-Natron and run portindex, which must report "Ports failed: 0"
```

Then register it **before** the default source. Add this line above the line ending in `[default]` (usually the last line: `rsync://… [default]`, or `https://… [default]`) of `/opt/local/etc/macports/sources.conf`:

```
file:///opt/MacPorts-Natron
```

for example with:

```Shell
grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf ||
    sudo perl -0pi -e 's|^(\S+\s+\[default\b[^\]]*\])|file:///opt/MacPorts-Natron\n$1|m' /opt/local/etc/macports/sources.conf
grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf || echo 'Error: add the line to sources.conf by hand'
```

Check it with `port file openimageio`, which should print a path under `/opt/MacPorts-Natron`.

MacPorts only installs a port from a binary archive if the port's own ports tree lists archive sites, and the overlay doesn't: the overlay ports are built from source. The exception is the `qt5` Portfile, whose subports other than `qt5-qtbase` are unchanged and come from MacPorts' archives where they exist (on macOS 14 they do). This matters for `qt5-qtwebengine`, a dependency of `py311-pyside2`: it needs a full Xcode to build, and the release SDK has only the Command Line Tools. Re-run `./sync.sh` whenever the files in `tools/MacPorts` change, including after a `git pull` or a branch switch, and before installing or upgrading ports: an out-of-date `PortIndex` makes MacPorts resolve the overlay ports with stale versions.

`./check.sh` (in the same directory; `--all` to check every overlay port, not only the installed ones) often reports obsolete Portfiles right after a `port selfupdate`: you can still use the committed overlay. It also warns when `/opt/MacPorts-Natron` differs from your checkout, meaning `./sync.sh` must be re-run. Reintegrating it with upstream is a maintainer task, see "Reintegrating upstream Portfiles into the overlay" in `Documentation/source/maintainers/build-macos.rst`.

#### 4. OSMesa and LLVM

Natron and the openfx-misc plugins use a mangled OSMesa (software OpenGL, used for headless rendering) installed in `/opt/osmesa`, with a static LLVM in `/opt/llvm`. Build both with [osmesa-install](https://github.com/devernay/osmesa-install) **before** installing the other ports. The Mesa 18 build needs `python` to be Python 2.7, which is only selected for this step.

```Shell
sudo mkdir -p /opt/osmesa /opt/llvm
sudo chown $(id -un):staff /opt/osmesa /opt/llvm
sudo port -v -N install python27 cmake autoconf automake libtool pkgconfig gsed
sudo port select --set python python27

cd $NATRON_SRC/..   # anywhere outside the Natron checkout (not in tools/MacPorts)
git clone https://github.com/devernay/osmesa-install.git
mkdir osmesa-install/build && cd osmesa-install/build
env LLVM_BUILD=1 MKJOBS=8 MACOSX_DEPLOYMENT_TARGET=14.0 ../osmesa-install.sh     # LLVM 6.0.1 into /opt/llvm, Mesa 18.3.6 into /opt/osmesa; no sudo

sudo port select --set python none
```

Use osmesa-install master: it has the LLVM and Mesa build fixes needed with recent macOS SDKs and Xcode versions (verified with Xcode 27.2).

**Mesa 25.0.7 (optional).** osmesa-install also builds Mesa 25.0.7 with LLVM 19.1.7, which gives thread-safe OSMesa and OpenGL 4.5 compatibility contexts to the Shadertoy plugin; Mesa 18.3.6 remains the default. It needs meson, ninja and a Python 3 with mako and PyYAML instead of Python 2.7 and gsed. Install it next to the default one, and select it for the Natron build with `OSMESA_PREFIX` and `LLVM_PREFIX`:

```Shell
sudo port -v -N install meson ninja py311-mako py311-yaml
sudo mkdir -p /opt/osmesa-25.0.7 /opt/llvm-19.1.7
sudo chown $(id -un):staff /opt/osmesa-25.0.7 /opt/llvm-19.1.7
cd $NATRON_SRC/../osmesa-install/build   # the osmesa-install checkout of the step above
env OSMESA_VERSION=25.0.7 OSMESA_PREFIX=/opt/osmesa-25.0.7 LLVM_PREFIX=/opt/llvm-19.1.7 \
    PYTHON=/opt/local/bin/python3.11 MKJOBS=8 MACOSX_DEPLOYMENT_TARGET=14.0 ../osmesa-install.sh   # builds LLVM 19.1.7 first; no sudo
# then build Natron (step 10) with: env OSMESA_PREFIX=/opt/osmesa-25.0.7 LLVM_PREFIX=/opt/llvm-19.1.7 ...
```

Don't use a MacPorts or Homebrew LLVM (`LLVM_SYSTEM=1`) for release builds: the Shadertoy plugin would then depend on their dylibs.

#### 5. Compiler

Natron and the plug-ins are compiled with Apple clang and MacPorts' OpenMP runtime, `libomp`:

```Shell
sudo port -v -N install libomp
sudo port setrequested libomp
```

The deprecated qmake build, the plug-ins' Makefiles and `NATRON_MACOS_CLANG=macports` need a MacPorts clang (with `-fopenmp`), and so do the release scripts if Apple clang can't build the C++ standard with `libomp` (`compiler-common.sh` warns then). The `tools/tart` SDK image has one, for these fallbacks. Install the version MacPorts uses by default on your OS:

| macOS | clang port |
|---|---|
| 26 and later | `clang-22` |
| 15 | `clang-18` |
| 10.7 to 14 | `clang-17` |
| 10.6 | `clang-11` |

```Shell
CLANG=17   # from the table above: 17 on macOS 14 (the baseline) and 10.7 to 13, 18 on 15, 22 on 26 and later (also export it next to NATRON_SRC)
sudo port -v -N install clang-$CLANG
sudo port setrequested clang-$CLANG
```

In those cases, `tools/jenkins/compiler-common.sh` uses the newest installed `clang-mp-*` that is not newer than the default in the table above, so older systems such as macOS 10.15 keep using clang-17.

On a new macOS or clang version, check that OpenMP programs, built with Apple clang and with the MacPorts clang, link against the system libc++ and MacPorts' libomp (`tools/tart/provision.sh` does the same):

```Shell
printf '#include <omp.h>\nint main(){ int n = 0;\n#pragma omp parallel for schedule(dynamic) reduction(+:n)\nfor (int i = 0; i < 4; ++i) n += i;\nreturn n == 6 && omp_get_max_threads() > 0 ? 0 : 1; }\n' > /tmp/t.cpp
/usr/bin/clang++ -Xpreprocessor -fopenmp -I/opt/local/include/libomp /tmp/t.cpp -o /tmp/t -L/opt/local/lib/libomp -lomp && otool -L /tmp/t
clang++-mp-$CLANG -stdlib=libc++ -fopenmp /tmp/t.cpp -o /tmp/t && otool -L /tmp/t
# expect /usr/lib/libc++.1.dylib and /opt/local/lib/libomp/libomp.dylib
```

#### 6. Build tools

```Shell
sudo port -v -N install gsed gawk coreutils findutils keychain tmux gzip bash wget bison flex doxygen \
    texinfo meson ninja help2man unzip tcl expect
```

#### 7. Libraries

```Shell
sudo port -v -N install dbus
sudo port -v -N install glib2 -x11
sudo port -v -N install libjpeg-turbo libpng tiff expat libmng boost
sudo port -v -N install cairo -x11
sudo port -v -N install x264 x265
sudo port -v -N install libvpx +highbitdepth
sudo port -v -N install libraw +gpl2

# OpenEXR 3 replaces ilmbase (ignore "not installed" errors)
sudo port -v -N uninstall ilmbase opencv4 field3d
sudo port -v -N install -f openexr

# OpenColorIO 2 and 1
sudo port -v -N install minizip-ng pystring
[ -e /opt/local/include/minizip-ng ] || sudo ln -s . /opt/local/include/minizip-ng
sudo sed -i '' -e 's/^find_dependency(zstd/#find_dependency(zstd/' /opt/local/lib/cmake/minizip-ng/minizip-ng-config.cmake
sudo port -v -N install opencolorio  -quartz -python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314
sudo port -v -N install opencolorio1 -quartz -python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314

# openfx-io (SeExpr 2.11 only, for SEEXPR_HOME)
sudo port -v -N install seexpr211

# openfx-arena
sudo port -v -N install librsvg -quartz
sudo port -v -N install poppler librevenge libcdr-0.1 sox libzip
sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
sudo port -v -N install ImageMagick +natron -x11
# ImageMagick 7, which openfx-arena uses since Natron 2.6 (static, in /opt/local/lib/ImageMagick7,
# so it doesn't conflict with ImageMagick 6 above)
sudo port -v -N install ImageMagick7 +natron -x11 -openexr

# openfx-gmic needs no extra port: its build downloads pocketfft, its FFT library

# openfx-io
sudo port -v -N install ffmpeg +gpl2 +highbitdepth +natronmini
# COMMERCIAL builds (NATRON_LICENSE=COMMERCIAL) only: LGPL FFmpeg, LibRaw and libheif (no x264/x265),
# installed in /opt/local/libexec next to the GPL ones (see "COMMERCIAL builds" in section 10)
sudo port -v -N install ffmpeg-lgpl libraw-lgpl libheif-lgpl
sudo port -v -N install openimageio +freetype +natron +opencolorio +openjpeg +python311 -python310 -python312 -python313 -python314
```

#### 8. Python 3.11, Qt 5 and PySide2

Python 3.10 is end-of-life and is no longer used (Python version policy, 2026-11-17): Qt 5 builds use Python 3.11 with Qt 5.15.19 and PySide2/Shiboken2 5.15.19 (MacPorts `python311`, `py311-pyside2`); Qt 6 builds use Python 3.13. To switch an existing install from Python 3.10, see [Switching from Python 3.10 to 3.11](#switching-from-python-310-to-311).

```Shell
sudo port -v -N install python311 py311-pip
sudo port -v -N install py311-numpy -gfortran
sudo port -v -N install py311-sphinx py311-sphinx_rtd_theme
sudo port -v -N install qt5 qt5-qtwebengine
sudo port -v -N install py311-pyside2   # its shiboken2 builds against the clang/llvm it depends on

sudo port select --set python python311
sudo port select --set python3 python311
sudo port select --set sphinx py311-sphinx
sudo pip-3.11 install qtpy==1.11.2

# links expected by the build scripts
PY=/opt/local/Library/Frameworks/Python.framework/Versions/3.11
sudo ln -sfn shiboken2 $PY/lib/python3.11/site-packages/shiboken2_generator/shiboken2-3.11
sudo ln -sfn shiboken2-3.11 /opt/local/bin/shiboken2
sudo ln -sfn Frameworks /opt/local/Library/lib
```

#### 9. Packaging tools (only needed to build the `.dmg`)

See [macOS packaging tools](#macos-packaging-tools) below.

#### 10. Build Natron and the DMG

```Shell
mkdir -p $HOME/Development/workspace
cd $NATRON_SRC/tools/jenkins
# NATRON_MACOS_MIN: the release target, NATRON_MACOS_RELEASE_MIN in tools/jenkins/macos-release.sh
env NATRON_MACOS_MIN=14.0 NOUPDATE=1 WORKSPACE=$HOME/Development/workspace DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL \
    GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=RB-2.6 \
    UNIT_TESTS=false BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 \
    COMPILE_TYPE=release MKJOBS=8 \
    ./launchBuildMain.sh
```

`launchBuildMain.sh` builds Natron with CMake (`tools/jenkins/build-natron.sh` configures it with the release options, runs `cmake --install` with `NATRON_INSTALL_LAYOUT=PORTABLE`, and stages the result for the installer script). Extra CMake options go in `NATRON_EXTRA_CMAKE_FLAGS`, e.g. `NATRON_EXTRA_CMAKE_FLAGS=-DNATRON_FAST_MATH=ON`. `NATRON_BUILD_SYSTEM=qmake` selects the deprecated qmake build, the only one with the Breakpad crash reporter (the CMake build implies `DISABLE_BREAKPAD=1`). It checks out Natron (`GIT_URL`, `GIT_BRANCH`) and the plug-ins from GitHub: local changes in `$NATRON_SRC` are not built, except for the `tools/` scripts and the overlay. To build local changes, see "Developing with the MacPorts SDK".

The `.dmg` ends up in `$HOME/Development/workspace/builds_archive/natron_github_RB2/1/`, named `Natron-<branch>-<date>-<commit>-macOS<version>-<arch>.dmg`, where `<version>` is the oldest macOS it runs on. **Release or development target.** With `NATRON_MACOS_MIN=14.0`, as above (release builds; `tools/tart/provision.sh` sets it from `NATRON_MACOS_RELEASE_MIN` in `tools/jenkins/macos-release.sh`), `tools/jenkins/common.sh` builds Natron and the plug-ins for macOS 14.0, and `launchBuildMain.sh` then runs `tools/tart/check-minos.sh` on the `.dmg`, which fails the build if a Mach-O file in it needs a newer macOS (the list is in `<dmg name>-minos.txt`; `tools/tart/natron-build.sh` checks it again on the host). This only works with ports built for 14.0 (install section 2). Without `NATRON_MACOS_MIN` (development builds), Natron and the plug-ins are built for the build Mac's own macOS, so that the plug-ins can bundle MacPorts' ordinary binary archives (their deploy step, `OFX_DEPLOY_STRICT_MINOS`, rejects a library built for a newer macOS than the target), and the DMG is named after it. An explicit `MACOSX_DEPLOYMENT_TARGET` wins over both. Use `UNIT_TESTS=true` to also run the render regression suite (Natron-Tests); this adds `-tests.txt` and `-unit_tests_failures.zip` next to the DMG. `BUILD_FROM=N` / `BUILD_TO=N` run only some of the six steps (checkout, plugins, Natron, installer, tests, archive).

**Check the DMG on a bare macOS 14: required before every macOS release.** A DMG that runs in the SDK VM may still depend on something that only the SDK has (a MacPorts library that wasn't bundled, the Command Line Tools, Python modules). With tart, `tools/tart/natron-test.sh Natron-….dmg` runs the full [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests) suite against the DMG's `NatronRenderer` in a throw-away clone of the stock `macos-sonoma-base` image, from which it removes the Command Line Tools and Homebrew, and writes `<dmg name>-tests.txt` and `<dmg name>-unit_tests_failures.zip` next to the DMG. **Attach both files to the GitHub release, next to the DMG**, as for v2.5.0 (e.g. `Natron-2.5.0-macOS12-x86_64-tests.txt`). Its exit status is 1 if a test fails that isn't a known failure: the known failures are listed in Natron-Tests (`known_failures/common.txt` and `macos.txt`, reported as `KNOWN FAIL`; a listed test that passes is reported as `UNEXPECTED PASS`, and the list must then be updated). Review the failures, and add the accepted ones to those lists, with their issue, through a Natron-Tests pull request (see `tools/tart/README.md`). Without tart, copy `Natron.app` and Natron-Tests to a Mac running macOS 14 without MacPorts, and run `runTests.sh` as shown in "Testing" in `Documentation/source/maintainers/build-macos.rst` (it needs a `gtimeout`; it sets a private, empty `NATRON_DISK_CACHE_PATH` itself unless one is given).

The plug-ins (openfx-misc, openfx-io, openfx-arena, openfx-gmic) are built with their CMake builds, with `-DOFX_DEPLOY_BUNDLES=ON` (`tools/jenkins/build-plugins.sh`); `PLUGINS_BUILD_SYSTEM=make` builds them with their Makefiles instead.

For a release, build a tag instead of a branch:

```Shell
# NATRON_MACOS_MIN: NATRON_MACOS_RELEASE_MIN in tools/jenkins/macos-release.sh
env NATRON_MACOS_MIN=14.0 NATRON_DEV_STATUS=STABLE RELEASE_TAG=2.6.0 NATRON_BUILD_NUMBER=1 NOUPDATE=1 WORKSPACE=$HOME/Development/workspace \
    DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL GIT_URL=https://github.com/NatronGitHub/Natron.git \
    UNIT_TESTS=true BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 COMPILE_TYPE=release MKJOBS=8 \
    ./launchBuildMain.sh
```

**COMMERCIAL builds.** `NATRON_LICENSE=COMMERCIAL` builds a DMG without any GPL component (#1302, #1177). It needs the overlay's subports `ffmpeg-lgpl`, `libraw-lgpl` and `libheif-lgpl` (install section 7), which install an LGPL FFmpeg (the same as `ffmpeg +highbitdepth +natronmini`, without `+gpl2` and libzvbi), a LibRaw without the demosaic packs and a libheif without the x264 and x265 encoders in `/opt/local/libexec/ffmpeg-lgpl`, `/opt/local/libexec/libraw-lgpl` and `/opt/local/libexec/libheif-lgpl`. openfx-io is built against the first two (`tools/jenkins/common.sh`), and `build-plugins.sh` replaces the libheif and LibRaw that openimageio links in the plug-in bundles with theirs (same version and ABI). Both `build-plugins.sh` and `build-OSX-installer.sh` then run `tools/jenkins/check-commercial-bundle.sh`, which fails the build if a bundled library is GPL, links a GPL library (x264, x265, Xvid, poppler, SoX, FFTW, ...), is an FFmpeg library whose licence (`avcodec_license()`, `ffmpeg -L`) is not LGPL 2.1 or later, is a LibRaw with a GPL demosaic pack, or is one of the Qt modules that are GPL-3 only (Qt Charts, Data Visualization, Network Authorization, Designer, ...) or their PySide2 module. `build-OSX-installer.sh` leaves these Qt frameworks and PySide2 modules out of COMMERCIAL DMGs: Natron doesn't use them. The check can also be run by hand on any app, bundle or directory: `tools/jenkins/check-commercial-bundle.sh Natron.app`. With tart: `tools/tart/natron-build.sh -l COMMERCIAL`.

Signing, notarization and the test suite are described in `Documentation/source/maintainers/build-macos.rst`.

#### 11. Keeping MacPorts up to date

Do this every few weeks, and before a release build. Each step is safe to stop after; the `sudo` steps need your password.

**MacPorts reads Natron's ports only from `/opt/MacPorts-Natron`, which is a copy of the `tools/MacPorts` directory of the checkout you last ran `./sync.sh` from.** If that was an old branch or another worktree, MacPorts silently uses its old Portfiles. So always sync from an up-to-date checkout of the branch you build (step 1), and check the copy (step 3).

1. **Update the checkout** of the branch you build (usually `RB-2.6`):

   ```Shell
   cd $NATRON_SRC && git switch RB-2.6 && git pull
   ```

2. **Use MacPorts, not Homebrew, in this terminal.** Run `port_switch` (see "MacPorts from scratch" above), or `export PATH=/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin; unset HOMEBREW_PREFIX HOMEBREW_CELLAR HOMEBREW_REPOSITORY`. Then check that none of these resolve to `/opt/homebrew` or `/usr/local`:

   ```Shell
   which port pkg-config cmake python3
   ```

3. **Update MacPorts and publish the overlay:**

   ```Shell
   sudo port selfupdate
   cd $NATRON_SRC/tools/MacPorts
   ./sync.sh
   diff -rq -x 'PortIndex*' $NATRON_SRC/tools/MacPorts /opt/MacPorts-Natron
   ```

   `./sync.sh` must end with `Ports failed: 0`, and the `diff` must print nothing.

4. **Check the overlay against upstream:**

   ```Shell
   ./check.sh
   ```

   It lists each overlay Portfile (or `files/` patch) whose upstream version changed, with a suggested command. If it lists anything, bring the overlay up to date first, as described in "Reintegrating upstream Portfiles into the overlay" in the Maintainer Guide (`Documentation/source/maintainers/build-macos.rst`), commit the result, and run `./sync.sh` again from that checkout. You can also skip this and keep using the committed overlay: it still works, it's just behind upstream.

   Some overlay ports are deliberately **pinned** to a version other than upstream's. Their `Portfile` is kept when reintegrating; only `Portfile.orig` follows upstream:

   | port | pinned version | reason |
   |---|---|---|
   | `graphics/libraw` | 0.22.3 (upstream: 0.22.2) + patch | the GPL demosaic packs (`+gpl2`) are restored by a Natron patch |
   | `graphics/opencolorio` | 2.6.0 (upstream: 2.5.1) | VFX Reference Platform CY2027; keep it until upstream reaches 2.6, and rev-bump `openimageio` when the OCIO minor version changes |
   | `print/freetype` | 2.15.0_1 (upstream: 2.15.0_0) + patch | backport of FreeType's unreleased fix 05f343a: 2.15.0 resets reflected transforms rotated within about 1.8° of 45°, 135°, 225° or 315° to the identity, so such text in openfx-arena's Text comes out unrotated (openfx-arena #86, #88); unpin when MacPorts ships a FreeType release that contains 05f343a (2.15.1 or later). Check that upstream has 05f343a before unpinning: `check.sh` only compares version and revision, so a rev-bump alone isn't enough. `sudo port -v upgrade outdated` picks up the revision bump |

5. **See what would be upgraded,** now that the overlay is published:

   ```Shell
   port outdated
   ```

6. **Upgrade the installed ports:**

   ```Shell
   sudo port -v upgrade outdated
   ```

   This can pull in new dependencies or variants, and can take hours when a big port (Qt, LLVM) changed. If it stops on a failure, upgrade the remaining ports one by one, so that one broken port doesn't block the others:

   ```Shell
   sudo -v; for p in $(port -q echo outdated | awk '{print $1}'); do echo "== $p"; sudo port -v upgrade "$p"; done 2>&1 | tee /tmp/natron-port-upgrade.log
   ```

   MacPorts never downgrades a port. If a pinned port was installed in a newer version, remove the inactive versions first (`port uninstall` refuses to choose when several versions are installed), then uninstall it and install it again with the variants of install section 7:

   ```Shell
   sudo port uninstall inactive
   sudo port -f uninstall libraw && sudo port -v -N install libraw +gpl2
   ```

   If `port installed ffmpeg` shows `+vmaf`, reinstall it the same way with `+gpl2 +highbitdepth +natronmini`.

7. **Re-apply the install fixups.** An upgrade of the ports concerned can undo them. Re-run the `ln`, `sed`, `port select` and `pip` commands of install sections 7 (the `minizip-ng` link and `sed`, the `libzip.pc` `sed`) and 8 (`port select` for `python`, `python3` and `sphinx`, `pip-3.11 install qtpy==1.11.2`, and the `shiboken2` and `/opt/local/Library/lib` links). They are safe to run again. In tart, the `fixups` step of `tools/tart/provision.sh` re-applies the links, `sed`s and `port select`s after every update (`qtpy` is installed by its `python_qt` step).

8. **Reclaim disk space:**

   ```Shell
   sudo port uninstall inactive
   sudo port clean --work --dist installed
   sudo port reclaim
   ```

   `port clean --work --dist installed` removes the build directories and the downloaded source archives of the installed ports, and keeps their logs. Don't delete `/opt/local/var/macports/software`: MacPorts needs those archives to deactivate and reactivate ports.

9. **Rebuild and test,** if the upgrade touched anything Natron or the plug-ins link to: `qt5`, `py311-pyside2`, `python311`, `boost`, `ffmpeg`, `openimageio`, `opencolorio`, `libraw`, `openexr`, `ImageMagick`, `ImageMagick7`, `poppler`, `cairo`, `freetype`, … Rebuild from a fresh build directory (install section 10, or "Developing with the MacPorts SDK" below), clear Natron's cache (`rm -rf ~/Library/Caches/INRIA/Natron`), and run the unit tests (see [Testing](#testing)). Before a release, also run the Natron-Tests regression suite on the new DMG on a bare macOS 14 (required, `tools/tart/natron-test.sh`, see install section 10), and attach its `-tests.txt` and `-unit_tests_failures.zip` to the release.

With the tart VMs (`tools/tart`), `tools/tart/natron-sdk.sh update` does steps 3 to 7 inside a copy of the SDK image: it installs the MacPorts version set by `MACPORTS_VERSION` in `tools/tart/provision.sh` and syncs the ports tree (instead of `selfupdate`), copies the host checkout's `tools/` into the VM (so update the host checkout first, step 1), runs `check.sh` (and only warns if the overlay is behind upstream: reintegrate on the host first), `./sync.sh`, reinstalls the pinned ports whose installed version differs from the overlay, upgrades the outdated ports one by one, installs `ImageMagick7 +natron` if the image doesn't have it yet, rebuilds from source the ports (and OSMesa/LLVM) built for a macOS newer than 14.0, if any (see "Building for an older macOS"; on the macOS 14 image there should be none), and re-applies the fixups. The previous image is kept as `natron-sdk-sonoma-prev`. See `tools/tart/README.md`.

#### Switching from Python 3.10 to 3.11

Until 2026-11-17 the SDK used Python 3.10, which is end-of-life. On a machine installed before, after the routine above (steps 1 to 6), install the Python 3.11 ports, switch `openimageio`'s Python bindings, and redo the selections and links of install sections 8 and 9:

```Shell
sudo port -v -N install python311 py311-pip
sudo port -v -N install py311-numpy -gfortran
sudo port -v -N install py311-sphinx py311-sphinx_rtd_theme
sudo port -v -N install py311-pyside2       # builds from source if there is no binary archive: allow an hour or more
sudo port -v -N install py311-dmgbuild      # only to build the .dmg
# -n: only openimageio is rebuilt (without it, --enforce-variants would apply the variants to its dependencies)
sudo port -n -v -N upgrade --enforce-variants openimageio +freetype +natron +opencolorio +openjpeg \
    +python311 -python310 -python312 -python313 -python314

sudo port select --set python python311
sudo port select --set python3 python311
sudo port select --set sphinx py311-sphinx
sudo pip-3.11 install qtpy==1.11.2

PY=/opt/local/Library/Frameworks/Python.framework/Versions/3.11
sudo ln -sfn shiboken2 $PY/lib/python3.11/site-packages/shiboken2_generator/shiboken2-3.11
sudo ln -sfn shiboken2-3.11 /opt/local/bin/shiboken2
sudo ln -sfn dmgbuild-3.11 /opt/local/bin/dmgbuild   # if py311-dmgbuild is installed
```

If you built Mesa 25.0.7 (install section 4), also `sudo port -v -N install py311-mako py311-yaml`; it only needs them to rebuild Mesa.

Check that the build sees Python 3.11: `python3 --version`, `ls -l /opt/local/bin/shiboken2`, and `port installed openimageio` (it must show `+python311`). `opencolorio` and `opencolorio1` are installed without Python bindings and need no change. Then rebuild Natron from a **fresh** build directory, with `-DPython3_EXECUTABLE=/opt/local/bin/python3.11` (a build directory configured with Python 3.10 keeps it in its CMake cache), and run the tests (step 9).

Once Natron builds and passes the tests, remove Python 3.10 and everything that uses it. Remove the inactive ports first: the variant switch leaves the old `openimageio +python310` installed but inactive, and it would still show up as a dependent of `python310`. Then check that `port rdependents python310` (recursive) lists only `py310-*` ports; if the active `openimageio` is listed, the variant switch above didn't happen, so don't go on:

```Shell
sudo port uninstall inactive
port rdependents python310
sudo port uninstall --follow-dependents python310
sudo port uninstall python3_select-310   # the python3 selector entry for 3.10, a dependency of python310
port echo leaves                         # review: unrequested ports nothing depends on any more
```

The `pip`, `cython` and `sphinx` selector entries for 3.10 belong to the `py310-*` ports and go away with them. Uninstall the ports listed by `port echo leaves` only if you don't need them (`sudo port uninstall leaves` removes them all, including build dependencies such as `clang-18` that a later rebuild of `py311-pyside2` would install again).

With tart, `tools/tart/natron-sdk.sh update` does all of this except the removal of Python 3.10 (`step_python_migrate` in `tools/tart/provision.sh`, then its `fixups` step).

#### After a macOS upgrade

A new major version of macOS (e.g. 15 to 26) needs a new MacPorts and a reinstall of every port: ports built for the previous macOS may not work any more, and `port selfupdate` doesn't fix that. This follows the [MacPorts migration procedure](https://trac.macports.org/wiki/Migration).

1. **Update the Command Line Tools** for the new macOS (install section 1; `softwareupdate --list` shows the versions offered), and `sudo xcode-select -s /Library/Developer/CommandLineTools`. A release build stays on macOS 14 with the Command Line Tools 15.3; on a newer macOS, set `buildfromsource always` in `macports.conf` (install section 2).

2. **Install the MacPorts `.pkg` for the new macOS** from <https://www.macports.org/install.php>. It keeps your `/opt/local/etc/macports/sources.conf`. Check that the overlay is still registered (otherwise redo install section 3):

   ```Shell
   grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf && port file openimageio
   ```

   `port file openimageio` must print a path under `/opt/MacPorts-Natron`.

3. **Use MacPorts, not Homebrew,** and update the checkout (steps 1 and 2 of "Keeping MacPorts up to date").

4. **Save the list of installed and requested ports,** after removing the inactive versions (otherwise they are reinstalled too), and keep a copy:

   ```Shell
   sudo port uninstall inactive
   port -qv installed > ~/myports.txt
   port echo requested | cut -d ' ' -f 1 | uniq > ~/requested.txt
   test -s ~/myports.txt && test -s ~/requested.txt && cp ~/myports.txt ~/requested.txt ~/Desktop/
   ```

   Don't go on unless the `test` succeeded.

5. **Uninstall all ports,** then clean up:

   ```Shell
   sudo port -fp uninstall installed
   sudo port reclaim
   ```

6. **Publish the overlay again,** so that the reinstall uses Natron's ports:

   ```Shell
   cd $NATRON_SRC/tools/MacPorts && ./sync.sh
   diff -rq -x 'PortIndex*' $NATRON_SRC/tools/MacPorts /opt/MacPorts-Natron
   ```

7. **Rebuild OSMesa** if you use it (install section 4). It must be built before the ports that need it, so do it now, before the reinstall.

8. **Reinstall the ports** with MacPorts' `restore_ports.tcl` script, which reinstalls them with the same variants. Download it outside the Natron checkout:

   ```Shell
   cd /tmp
   curl --location --remote-name https://github.com/macports/macports-contrib/raw/master/restore_ports/restore_ports.tcl
   chmod +x restore_ports.tcl
   sudo ./restore_ports.tcl ~/myports.txt
   sudo port unsetrequested installed
   xargs sudo port setrequested < ~/requested.txt
   ```

   (A file downloaded with a web browser instead of `curl` also needs `xattr -d com.apple.quarantine restore_ports.tcl`.) If a port fails, reinstall it by hand with the commands and variants of install sections 4 to 9, which are the reference for Natron's variants (`ImageMagick +natron`, `ImageMagick7 +natron`, `ffmpeg +natronmini`, `libraw +gpl2`, ...).

9. **Re-apply the install fixups** (step 7 of "Keeping MacPorts up to date"): `restore_ports.tcl` doesn't restore `port select` choices, `pip` packages or hand-made links.

10. **Check the compiler** (install section 5), then rebuild from a fresh build directory and test (step 9 of "Keeping MacPorts up to date"). A clean rebuild is mandatory after a macOS upgrade.

#### Building for an older macOS

The release supports macOS 14 and later (see install section 2). On macOS 14 itself, MacPorts' binary archives are built for 14.0 and nothing needs rebuilding. On a newer macOS, ports installed from MacPorts' binary archives, or before `macosx_deployment_target 14.0` and `buildfromsource always` were set in `macports.conf`, are built for the macOS they were installed on, and their dylibs make the `.dmg` require that macOS. `tools/tart/check-minos.sh` (`-t TARGET`, default 14.0) lists the Mach-O files that need a newer macOS than the target. To rebuild every port that has one, from source and in dependency order:

```Shell
CHECK=$NATRON_SRC/tools/tart/check-minos.sh
for p in $(port -q installed active | awk '{print $1}'); do
    port -q contents $p | $CHECK -t 14.0 -l > /dev/null 2>&1; [ $? = 1 ] && echo $p
done > ~/rebuild.txt
sudo port -n -s upgrade --force $(cat ~/rebuild.txt)   # one port at a time is also fine
```

`-n` rebuilds only the given ports, `-s` builds from source, and `--force` rebuilds ports that are up to date. The ports are rebuilt in the order given: putting the dependencies first is safer (a port may link a static library or inline a header of a dependency), and the tart step below does that. Rebuild OSMesa and LLVM too (install section 4, with `MACOSX_DEPLOYMENT_TARGET=14.0`): `$CHECK /opt/osmesa/lib /opt/llvm/lib/libLLVMSupport.a` tells whether they need it. A few ports may stay too new after a rebuild (prebuilt binaries, or a build system that ignores `MACOSX_DEPLOYMENT_TARGET`): that only matters if their files end up in the `.dmg`, which `check-minos.sh` on the `.dmg` tells. `tools/tart/natron-sdk.sh update` does all of this in the tart SDK image (`step_macports_conf` and `step_rebuild_target` in `tools/tart/provision.sh`).

#### Developing with the MacPorts SDK

`launchBuildMain.sh` builds everything from fresh checkouts. To work on Natron or the plugins directly, build them from your own checkouts.

The OpenFX plugins ([openfx-io](https://github.com/NatronGitHub/openfx-io), [openfx-misc](https://github.com/NatronGitHub/openfx-misc), [openfx-arena](https://github.com/NatronGitHub/openfx-arena), [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic)) build the way `tools/jenkins/build-plugins.sh` builds them: openfx-misc, openfx-io and openfx-arena with CMake (#1203, #1280), openfx-gmic with `make`. The bundles go to `~/OFX/Natron-release`, which is the directory that `OFX_PLUGIN_PATH` points to in the commands below.

```Shell
mkdir -p ~/src && cd ~/src
for p in openfx-io openfx-misc openfx-arena openfx-gmic; do git clone --recursive https://github.com/NatronGitHub/$p.git; done
```

openfx-misc, openfx-io and openfx-arena use openfx's CMake helpers. Build them with Apple clang and MacPorts' `libomp` (`OFX_LIBOMP_ROOT`, as `build-plugins.sh` does; `-DCMAKE_CXX_COMPILER=clang++-mp-$CLANG -DCMAKE_CXX_FLAGS=-stdlib=libc++` works too), as C++17 with a Qt5 Natron (`-DOFX_CXX_STANDARD=20` with Qt6), and install the bundles straight into `~/OFX/Natron-release`. openfx-arena needs the ImageMagick 7 of install section 7 (`ImageMagick7 +natron`, in `/opt/local/lib/ImageMagick7`): its pkg-config files must come first, as `build-plugins.sh` does with `MAGICK7_PREFIX`, or pkg-config finds the ImageMagick 6 of `/opt/local` instead.

```Shell
for p in openfx-misc openfx-io openfx-arena; do
  cd ~/src/$p
  env PKG_CONFIG_PATH=/opt/local/lib/ImageMagick7/lib/pkgconfig \
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DOFX_LIBOMP_ROOT=/opt/local \
      -DCMAKE_PREFIX_PATH=/opt/local -DSEEXPR_HOME=/opt/local -DOFX_CXX_STANDARD=17 \
      -DPLUGIN_INSTALLDIR=$HOME/OFX/Natron-release
  cmake --build build && cmake --install build
done
```

CMake's `Release` uses `-O3`, and `build-plugins.sh` uses `-O3 -g` for its `RelWithDebInfo` builds, without fast math (`-Ofast` would break the plug-ins' NaN and infinity checks). `build-plugins.sh` also passes `-DOFX_DEPLOY_BUNDLES=ON`, which copies the MacPorts libraries into the bundles for the DMG; a development build doesn't need it. `SEEXPR_HOME` is only used by openfx-io, and `PKG_CONFIG_PATH` only by openfx-arena: the others ignore them (openfx-misc warns that `SEEXPR_HOME` is unused). For CPU rendering in Shadertoy, add `-DUSE_OSMESA=ON` for openfx-misc. It uses the OSMesa and LLVM of install section 4: `/opt/osmesa` and `/opt/llvm` by default, or set `-DOSMESA_PATH` and `-DLLVM_PATH`. openfx-arena builds with OpenMP, as Natron's ImageMagick 7 does. The CMake options are listed in each repository's `README.md` and `CMakeLists.txt`.

openfx-gmic builds with `make` (`CONFIG=debug` or `release`, OpenMP with the MacPorts clang, no OpenGL), at `-O3` like the CMake builds:

```Shell
cd ~/src/openfx-gmic
env OPENMP=1 CXX="clang++-mp-$CLANG -stdlib=libc++ -std=c++17" OPTFLAG=-O3 \
    CXXFLAGS_EXTRA="-isysroot $(xcrun --sdk macosx --show-sdk-path)" \
    CONFIG=release make -j8
```

Use `-std=c++17` with a Qt5 Natron and `-std=c++20` with a Qt6 Natron (RB-2.6 policy; the packaging scripts do this automatically).

The bundle is written to `GMIC_OFX/Darwin-64-release/GMIC.ofx.bundle`. To use it, symlink it next to the others (or use `/Library/OFX/Plugins`):

```Shell
ln -sf ~/src/openfx-gmic/GMIC_OFX/Darwin-64-release/GMIC.ofx.bundle ~/OFX/Natron-release/
```

Natron itself builds with CMake, the system (Apple) clang and the MacPorts libraries (see [Building with CMake](#building-with-cmake) below), from `$NATRON_SRC`, with Homebrew off the `PATH`:

```Shell
cd $NATRON_SRC
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DNATRON_BUILD_TESTS=ON \
    -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
    -DPython3_EXECUTABLE=/opt/local/bin/python3.11
cmake --build build -j$(sysctl -n hw.ncpu)
```

The application is then `build/App/Natron.app`. The release scripts add the options of the official builds: `-DNATRON_BUILD_CONFIG=SNAPSHOT` (or `ALPHA`, `BETA`, `RC`, `STABLE`), `-DNATRON_USE_OPENMP=ON` (with MacPorts' `libomp`), `-DNATRON_NO_ASSERTIONS=ON` and `-DNATRON_INSTALL_LAYOUT=PORTABLE` (see `Documentation/source/maintainers/building.rst`). The deprecated qmake build, still available on RB-2.6, uses the command below (see [Deprecated: building with qmake](#deprecated-building-with-qmake) for `config.pri`).

```Shell
cd $NATRON_SRC
cp config-macports.pri config.pri
/opt/local/libexec/qt5/bin/qmake -r CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa \
    CONFIG+=openmp CONFIG+=python3 CONFIG+=sdk_no_version_check CLANG_MP_VERSION=$CLANG
QT_MAC_SDK_NO_VERSION_CHECK=1 make -j8
```

`CLANG_MP_VERSION` selects the MacPorts clang used with `CONFIG+=openmp` (see `config-macports.pri`).

The OCIO configuration comes from the OpenColorIO-Configs download (see [Download OpenColorIO-Configs](#download-opencolorio-configs) below). To run a development build, or the [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests) suite (`runTests.sh <path to NatronRenderer>`), set the environment that the packaged application normally provides (the CMake build generates `fonts.conf` in the build directory, from `Gui/Resources/etc/fonts/fonts.conf.in`), and `NATRON_DISK_CACHE_PATH` to a private directory for test runs (the default disk cache is shared by every build of the version):

```Shell
env FONTCONFIG_FILE=$NATRON_SRC/build/App/Resources/etc/fonts/fonts.conf \
    OCIO=$NATRON_SRC/OpenColorIO-Configs/blender/config.ocio \
    OFX_PLUGIN_PATH=~/OFX/Natron-release NATRON_PLUGIN_PATH=$NATRON_SRC/Gui/Resources/PyPlugs \
    build/App/Natron.app/Contents/MacOS/Natron
```

(with the qmake build, the application is `App/Natron.app`).

Clear the OFX plugin cache (`~/Library/Caches/INRIA/Natron`) after rebuilding plugins. The plugin reference pages of the documentation are generated with `tools/genStaticDocs.sh` (see `Documentation/README-Natron-documentation.md`).

#### Notes for older systems

Natron 2.6 runs on macOS 14 or later, on arm64 and x86_64. The x86_64 (Intel) builds are made with the MacPorts steps above on an Intel Mac, or in a macOS VM on one: due to the macOS Software License Agreement, macOS may only be virtualized on Mac hardware. To keep such a VM small, keep only macOS and the Command Line Tools in it and put `/opt` on a sparsebundle disk image stored on the host. Create it on the host with Disk Utility, using a name without spaces (MacPorts doesn't support paths with spaces), e.g. `opt`, with a maximum size of about 100 GB. In the guest, mount the host share (`open smb://<host-ip>/<user>`), attach the image with `sudo hdiutil attach -owners on /Volumes/<user>/path/to/opt.sparsebundle`, and link it: `sudo rmdir /opt; sudo ln -s /Volumes/opt /`.

The notes for building on much older systems (OS X 10.6 universal builds, libc++ on older systems, OS X 10.9 VMs with Xcode 6.2) are in the RB-2.5 branch of `INSTALL_MACOS.md`.

## macOS packaging tools

In addition to the compile-time dependencies above, building the distributable
`.dmg` with `tools/jenkins/launchBuildMain.sh` requires a few extra tools. They
are verified early by the build script (the `PKGOS = OSX` check in
`launchBuildMain.sh`), so a missing tool fails fast with a clear message instead
of after a multi-hour build:

- **dmgbuild** — writes the DMG window background and icon layout directly into
  the disk image's `.DS_Store`, without driving Finder (the old AppleScript
  approach needs GUI Automation / TCC permission and fails in headless or
  unprivileged sessions). With MacPorts, install the port and link the
  versioned script (the port only installs `dmgbuild-3.11`):

  ```Shell
  sudo port -v -N install py311-dmgbuild
  sudo ln -sfn dmgbuild-3.11 /opt/local/bin/dmgbuild
  ```

  Otherwise, install it with pip: `python3 -m pip install dmgbuild`.

- **oiiotool** — provided by the `openimageio` port (already installed as a
  Natron dependency); used to contrast-reduce the DMG background image.
- **identify** — provided by the `ImageMagick` port (already installed as a
  Natron dependency); used to read the background image dimensions.

If you add a new macOS build/packaging dependency, add it to the early check in
`launchBuildMain.sh` and document it here.

## Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz).
Place it at the root of Natron source tree:

```Shell
curl -k -L https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz | tar zxf -
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```

## Building with CMake

CMake is the supported build. It builds with Qt 5 and PySide2 by default; `-DNATRON_QT6=ON` selects Qt 6 and PySide6 (not covered by the macOS CI; following the Python version policy, use Python 3.13 with it, i.e. MacPorts `python313` and `py313-pyside6`, and `-DPython3_EXECUTABLE=/opt/local/bin/python3.13`). Other options: `-DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo`, `-DNATRON_BUILD_TESTS=OFF` (skip the unit tests), `-DNATRON_SYSTEM_LIBS=ON` (use system copies of the libraries in `libs/`). `cmake --install build --prefix <dir>` installs Natron; `-DNATRON_INSTALL_LAYOUT=PORTABLE` installs a self-contained `Natron.app`, as the release scripts will (see `Documentation/source/maintainers/building.rst`).

### On MacPorts

MacPorts' `py311-pyside2` has no CMake config files: CMake then finds Shiboken2 and PySide2 through `Python3_EXECUTABLE` (see `cmake/PySide2FromPython.cmake`). Build with the system (Apple) clang and the MacPorts libraries, from `$NATRON_SRC`, with Homebrew off the `PATH`:

```Shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
    -DPython3_EXECUTABLE=/opt/local/bin/python3.11
cmake --build build -j$(sysctl -n hw.ncpu)
(cd build && env OFX_PLUGIN_PATH=$HOME/OFX/Natron-release OCIO=$NATRON_SRC/OpenColorIO-Configs/blender/config.ocio ctest -V)
```

These are the macOS CI job's commands (`.github/workflows/ci.yml`), except that CI has no OFX plug-ins: it leaves `OFX_PLUGIN_PATH` unset and skips the tests that need them with `GTEST_FILTER='-BaseTest.*'` in the `ctest` environment.

The shiboken2 generator parses the headers with its own libclang (LLVM 18 for PySide2 5.15), which can't parse the libc++ headers of newer SDKs (`use of undeclared identifier '__builtin_clzg'`, seen with the macOS 27 SDK of Xcode 27). If that happens, point `SDKROOT` at an older SDK, e.g. `export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk`. It must be exported in the environment of `cmake --build` too, not only at configure time: shiboken runs during the build, and without `SDKROOT` it picks the Xcode SDK headers again. The same applies to the qmake build and to `launchBuildMain.sh`.

### Xcode

Natron ships no Xcode project: generate one with CMake, then open it:

```Shell
cmake -S . -B build-xcode -G Xcode \
    -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
    -DPython3_EXECUTABLE=/opt/local/bin/python3.11
open build-xcode/Natron.xcodeproj
```

The compiler options are not needed: the project uses Xcode's clang. Natron's debug definitions (`DEBUG`, `qDebug()` output) follow the Debug or Release configuration chosen in Xcode.

The Xcode generator needs a full Xcode and uses its SDK. With `SDKROOT` or `CMAKE_OSX_SYSROOT` set to a Command Line Tools SDK, CMake finds no compiler. So the `SDKROOT` workaround above for shiboken2 doesn't work here: if the Python bindings fail to generate, use the Makefile or Ninja build.

To build the plug-ins in Xcode, generate a project with their CMake build in the same way (`cmake -S . -B build-xcode -G Xcode`). Older plug-in sources ship hand-made Xcode projects, which need "Source Trees" variables set in Xcode's preferences: use CMake instead.

## Deprecated: building with qmake

*The qmake build (Qt 5 only) is deprecated. It still works on RB-2.6, where the release scripts in `tools/jenkins` use it with `NATRON_BUILD_SYSTEM=qmake`, but it will be removed in a future release (first on RB-2.7, see [#1181](https://github.com/NatronGitHub/Natron/issues/1181)).*

### Add the config.pri file

You have to define the locations of the required libraries.
This is done by creating a .pri file that will tell the .pro where to find those libraries.
The only library to put in the config.pri file on unix systems is boost.
For all other libraries are found with PKGConfig.

- create the config.pri file next to the Project.pro file.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

INCLUDEPATH is the path to the include files

LIBS is the path to the libs

If you installed libraries using MacPorts, use the following
config.pri:

```Shell
# copy and paste the following in a terminal
cp config-macports.pri config.pri
```

### Build with Makefile

You can generate a makefile by opening a Terminal, setting the current
directory to the toplevel source directory, and typing

```Shell
qmake -r
```

then type

```Shell
make
```

This will create all binaries in all the subprojects folders.

If you want to build in DEBUG mode change the qmake call to this line:

```Shell
qmake -r CONFIG+=debug
```

* You can also enable logging by adding CONFIG+=log

* You can also enable clang sanitizer by adding CONFIG+=sanitizer

## Testing

With CMake, the unit tests are built with Natron (`NATRON_BUILD_TESTS`, on by default) and run with `ctest` (see [Building with CMake](#building-with-cmake)).

With the deprecated qmake build, they build with the same qmake as Natron (see "Developing with the MacPorts SDK"), from `$NATRON_SRC`:

```Shell
cd $NATRON_SRC/Tests
# the top-level build (see "Developing with the MacPorts SDK") also builds Tests;
# the tests need OFX plug-ins (e.g. openfx-io) and an OCIO config:
env OFX_PLUGIN_PATH=$HOME/OFX/Natron-release OCIO=$NATRON_SRC/OpenColorIO-Configs/blender/config.ocio ./Tests
# for coverage, re-run the top-level qmake with the same flags plus CONFIG+=coverage
```

The render regression tests are in [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests); `launchBuildMain.sh` runs them with `UNIT_TESTS=true`.

## Generating Python bindings

The CMake build generates the Python bindings during the build: nothing to do.

The qmake build doesn't: the generated sources (`Engine/Qt5/NatronEngine` and `Gui/Qt5/NatronGui`) must be regenerated with `shiboken2` whenever the `Py*` facades or the typesystem XML files change, followed by `tools/utils/sourceCleanup.py`, as `tools/jenkins/build-natron.sh` does. From `$NATRON_SRC`, on MacPorts with qt5 and py311-pyside2:
```Shell
PYVER=3.11 # Set to the python version
PYTHON_HOME=$(python${PYVER}-config --prefix)
QT_VERSION_MAJOR=5
QTDIR=/opt/local/libexec/qt${QT_VERSION_MAJOR}
# Fix a missing link in the MacPorts package
[ ! -f ${PYTHON_HOME}/lib/python${PYVER}/site-packages/shiboken2_generator/shiboken2-${PYVER} ] && sudo ln -s shiboken2 ${PYTHON_HOME}/lib/python${PYVER}/site-packages/shiboken2_generator/shiboken2-${PYVER}

rm Engine/Qt${QT_VERSION_MAJOR}/NatronEngine/* Gui/Qt${QT_VERSION_MAJOR}/NatronGui/*
SHIBOKEN_INCLUDE_PATHS="-I. -I./Engine -I./Global -Ilibs/OpenFX/include -I/opt/local/include -I${QTDIR}/include -I${QTDIR}/include/QtCore -I${PYTHON_HOME}/include/python${PYVER} -I${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include -I${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include/QtCore -I${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include/QtGui"
SHIBOKEN_TYPESYSTEM_PATHS="-T${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/typesystems"
shiboken2-${PYVER} -std=c++17 --avoid-protected-hack --enable-pyside-extensions ${SHIBOKEN_INCLUDE_PATHS} ${SHIBOKEN_TYPESYSTEM_PATHS} --output-directory=Engine/Qt${QT_VERSION_MAJOR} Engine/PySide2_Engine_Python.h Engine/typesystem_engine.xml
shiboken2-${PYVER} -std=c++17 --avoid-protected-hack --enable-pyside-extensions ${SHIBOKEN_INCLUDE_PATHS} -I${QTDIR}/include/QtWidgets -I${QTDIR}/include/QtGui ${SHIBOKEN_TYPESYSTEM_PATHS} -T./Engine -T./Shiboken --output-directory=Gui/Qt${QT_VERSION_MAJOR} Gui/PySide2_Gui_Python.h Gui/typesystem_natronGui.xml

# Fix the glitches of the generated sources
python3 tools/utils/sourceCleanup.py Engine/typesystem_engine.xml Engine/Qt${QT_VERSION_MAJOR}
python3 tools/utils/sourceCleanup.py Gui/typesystem_natronGui.xml Gui/Qt${QT_VERSION_MAJOR}
```

## OpenFX plugins

With MacPorts, build the plugins as in "Developing with the MacPorts SDK" above, or as `tools/jenkins/build-plugins.sh` does. Each plugin repository's `README.md` has its own build instructions, and its CI workflows in `.github/workflows` show a complete build.
