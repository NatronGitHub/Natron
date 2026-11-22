# Automated macOS builds with tart

These scripts build Natron for macOS in [tart](https://tart.run) virtual machines on an Apple Silicon Mac. They are the macOS counterpart of `tools/docker`:

| | Docker (Linux) | tart (macOS) |
|---|---|---|
| SDK image (all build dependencies) | `natron-sdk` | `natron-sdk.sh` → local image `natron-sdk-sonoma` |
| build | `natron-build` | `natron-build.sh` |
| regression tests of a DMG on a bare macOS | | `natron-test.sh` |

The SDK image runs **macOS 14 Sonoma**, the oldest macOS that Natron 2.6 supports and the distribution baseline (maintainer decision, 2026-11-21), and everything in it is built for macOS 14.0. It starts from the stock `ghcr.io/cirruslabs/macos-sonoma-base` image (pinned by digest in `tools/jenkins/macos-release.sh`), and has **no Xcode**: only the Command Line Tools for Xcode 15.3, with the macOS 14.4 SDK and Apple clang 15.0.0. Since it runs macOS 14, MacPorts' binary archives (built for the macOS they were made on) are used where they exist; MacPorts has `macosx_deployment_target 14.0` in `macports.conf`, OSMesa/LLVM are built with `MACOSX_DEPLOYMENT_TARGET=14.0`, and `tools/jenkins/common.sh` builds Natron and the plug-ins for 14.0. The SDK is installed with the same procedure as the MacPorts section of `INSTALL_MACOS.md`, which describes every step from the base image: Homebrew removed, Command Line Tools, MacPorts + the `tools/MacPorts` overlay, OSMesa/LLVM via [osmesa-install](https://github.com/devernay/osmesa-install), the MacPorts clang matching the OS, libraries, Qt5/PySide2, and the DMG tools. (GitHub's CI uses `macos-15` runners, because GitHub has no macOS 14 runners; the release is built here, on macOS 14.)

## Requirements

**The host must be a Mac with Apple Silicon.** tart is a thin layer over Apple's Virtualization.framework, so it does not run on Intel Macs, Linux or Windows. The guests it can run are macOS 12 or later (arm64) and arm64 Linux. That means:

- These scripts only produce **arm64 macOS** builds. Intel/universal macOS builds, and builds or tests on macOS 11 or older (e.g. 10.15), need an Intel Mac or a VM on one (see the notes for older systems in `INSTALL_MACOS.md`).
- Linux and Windows releases are not built here: use `tools/docker` (Linux) and the MSYS2 builds (Windows).

Other requirements:

- `tart` (`brew install cirruslabs/cli/tart`).
- rsync 3 (`brew install rsync` or `sudo port install rsync`). macOS's own `/usr/bin/rsync` (openrsync) can't copy to IPv6 addresses.
- `sshpass`, used once to install the SSH key (`brew install hudochenkov/sshpass/sshpass`).
- About 30 GB to download the base image the first time (`natron-sdk.sh create` pulls the pinned image), plus up to `VM_DISK` (250 GB, sparse) per VM. The finished SDK image takes about 30 GB on disk (`du -sh ~/.tart/vms/natron-sdk-sonoma`). With 6 CPUs, `natron-sdk.sh create` took 70 minutes on 2026-11-21 (most ports come from binary archives; libs 26 and python_qt 13 minutes), `natron-build.sh` 26 minutes, and `natron-test.sh` about 10 minutes.
- The Natron checkout containing these scripts. Optionally, `../osmesa-install` next to it (its working copy is used); otherwise it is cloned from GitHub inside the VM.

## Build or update the SDK image

```sh
tools/tart/natron-sdk.sh create          # from the cirruslabs base image; resumable; takes hours
tools/tart/natron-sdk.sh update [STEP…]  # MacPorts pkg if MACPORTS_VERSION changed, port sync, overlay,
                                         # pinned ports, upgrade outdated ports; STEPs are re-run
tools/tart/natron-sdk.sh push ghcr.io/<org>/natron-sdk-sonoma:<tag>   # optional, to share it
```

Both `create` and `update` work on a copy named `natron-sdk-sonoma-wip`. The image is replaced only when every step succeeded, and the previous image is kept as `natron-sdk-sonoma-prev`.

If a step fails, the `-wip` VM is left running. Fix the cause on the host (for example in `tools/MacPorts`), then resume with `natron-sdk.sh create`, for both `create` and `update` (an interrupted update is finished first): steps that already succeeded are skipped, and `REDO="step…"` re-runs some of them. `update` re-runs only new steps and the steps you name: after adding a port to an existing step (e.g. `libs`), run `natron-sdk.sh update libs`. The logs are in `~admin/provision-logs/<step>.log` inside the VM. To look at them, source `tart-common.sh` and run `vm_ssh natron-sdk-sonoma-wip`.

`create` starts with the steps that make the base image usable (see sections 0 and 1 of `INSTALL_MACOS.md`): `nobrew` removes Homebrew (the tart guest agent it installed is moved to `/usr/local/libexec`), and `clt` replaces the base image's Command Line Tools 16.2 (macOS 15.2 SDK) with the Command Line Tools for Xcode 15.3 (macOS 14.4 SDK), using `softwareupdate` (no Apple ID needed) with the system SOCKS proxy set for the install.

`create` and `update` also rebuild, from source and in dependency order, every installed port that has a Mach-O file built for a macOS newer than 14.0 (`step_rebuild_target`), and OSMesa/LLVM if they are (`step_osmesa`). On the macOS 14 image this should find nothing: it is a safety net for binary archives built for a newer macOS, and for images built on a newer macOS (where `macports.conf` also gets `buildfromsource always`). A port still too new after its rebuild is listed in `~admin/provision-logs/rebuild_target.skip` and not rebuilt again until its version changes.

Before `update`, reintegrate upstream MacPorts changes into the overlay on the host (`tools/MacPorts/check.sh`, see `tools/MacPorts/skills/macports-natron-upgrade/SKILL.md`). The `update` step only warns when the overlay is out of date.

## Build Natron

```sh
tools/tart/natron-build.sh                 # RB-2.6, release, no tests
tools/tart/natron-build.sh -b RB-2.6 -t    # also run the unit/regression tests
tools/tart/natron-build.sh -s qmake        # build Natron with the deprecated qmake build
tools/tart/natron-build.sh -l COMMERCIAL   # a COMMERCIAL DMG, without GPL components (needs the ffmpeg-lgpl, libraw-lgpl and libheif-lgpl subports)
```

At the end, `check-minos.sh` checks that every Mach-O file in the DMG (the executables, the bundled dylibs and frameworks, the Python framework and modules, and the OFX bundles) has a minimum macOS (`minos`) of 14.0 or older (`MACOS_MIN`). The build fails otherwise, and the files are listed in `<dmg name>-minos.txt` next to the DMG. `check-minos.sh [-t TARGET] [-v] PATH…` also works on directories, files, or any other DMG.

Natron is built with CMake (`NATRON_BUILD_SYSTEM=cmake`, see `tools/jenkins/build-natron.sh`); `NATRON_EXTRA_CMAKE_FLAGS` in the environment is passed on to it.

Each build runs in a fresh clone of the SDK image. Clones are copy-on-write, so they are fast and small. The clone is deleted afterwards (`-k` keeps it, shut down). The results (`.dmg`, test results, logs) are copied to `./natron-builds/<vm-name>/`.

The files tracked by git under `tools/` are copied into the VM with their local changes, so uncommitted changes to `tools/jenkins` are tested. Changes to the `tools/MacPorts` overlay are published (`portindex`) but installed ports are not rebuilt: run `natron-sdk.sh update` for that. The Natron and plugin sources are checked out from GitHub by `launchBuildMain.sh`.

Only one Natron VM can run at a time: clones of an image share their IPv6 link-local address, so the scripts refuse to start a VM while another `natron-sdk-*`, `natron-build-*` or `natron-test-*` VM is running.

## Test a DMG on a bare macOS 14

**Required before every macOS release**, and the two result files are attached to the GitHub release next to the DMG, as for v2.5.0.

```sh
tools/tart/natron-test.sh natron-builds/<vm>/natron_github_RB2/1/Natron-*.dmg   # the full Natron-Tests suite
TEST=TestBlur tools/tart/natron-test.sh Natron-….dmg                            # a single test (results named <dmg>-partial-TestBlur-*)
```

This runs the [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests) regression suite (`runTests.sh`, as `tools/jenkins/runUnitTests.sh` does) against the DMG's `NatronRenderer`, in a throw-away clone of the stock base image (`BASE_IMAGE`), not of the SDK image. In that clone the Command Line Tools are removed and Homebrew is moved away: no Xcode, no Command Line Tools, no MacPorts, no Homebrew, no Natron sources. Only `Natron.app`, copied from the DMG, and the Natron-Tests data are copied in, so a missing or too-new bundled library, or a missing resource (OCIO configs, fonts, Python modules) shows up. The tests use the `ffmpeg` and `idiff` bundled in `Natron.app`, macOS's `/bin/bash`, `curl` and `unzip`, and a small Perl `gtimeout`. The Spaceship and BayMax example projects that `runTests.sh` needs are downloaded from SourceForge once, on the host, into the Natron-Tests checkout. The Natron-Tests checkout is `$NATRON_TESTS_SRC` (default: `Natron-Tests` next to the Natron checkout, cloned if missing; `-n DIR`).

The results are written next to the DMG (`-o DIR` elsewhere), named as the release assets of previous versions: `<dmg name>-tests.txt` (one `Test : PASS`, `FAIL`, `KNOWN FAIL` or `UNEXPECTED PASS` line per test), `<dmg name>-tests-summary.txt` (the counts), `<dmg name>-unit_tests_failures.zip` (the failed tests' outputs and references, in a `<dmg name>-unit_tests_failures/` directory), and `<dmg name>-tests.log` (the output of `runTests.sh`). The clone is deleted afterwards (`-k` keeps it).

Known failures are listed in Natron-Tests itself, in `known_failures/common.txt` (all platforms) and `known_failures/macos.txt` (see its `known_failures/README.md`); `natron-test.sh` stops if the Natron-Tests checkout has no `known_failures/common.txt`. `runTests.sh` reports the known failures as `KNOWN FAIL`, and a listed test that passes as `UNEXPECTED PASS` (the list must then be updated, and the warning says so). The exit status is 1 if a test fails that isn't a known failure, or if the suite didn't run; unexpected passes don't fail the run. A failure accepted for a release goes into Natron-Tests' lists, with its issue, through a Natron-Tests pull request. `runTests.sh` uses a private, empty disk cache (`NATRON_DISK_CACHE_PATH`), so the results don't depend on what other builds left in the default cache.

## Settings

Environment variables (see `tart-common.sh`): `BASE_IMAGE` (default `NATRON_MACOS_BASE_IMAGE` in `tools/jenkins/macos-release.sh`, pinned by digest for reproducible release images; `BASE_IMAGE=ghcr.io/cirruslabs/macos-sonoma-base:latest` tries the newest one), `SDK_IMAGE`, `VM_CPU` (8), `VM_MEMORY` (16384 MB), `VM_DISK` (250 GB), `TART_KEY` (`~/.ssh/natron-tart_ed25519`), `OSMESA_INSTALL_SRC`, `NATRON_TESTS_SRC`, `TEST_TIMEOUT` (14400 s, natron-test.sh), `GUEST_PROXY_ALLOW` (`*:80 *:443`), and `RSYNC`. `MACPORTS_VERSION` and the Python version (`pyver`, 3.11) are at the top of `provision.sh`; the macOS target (14.0), the Command Line Tools version (15.3) and the base image are in `tools/jenkins/macos-release.sh`. On an SDK image made with an older Python, `natron-sdk.sh update` installs the new Python ports and switches `openimageio`'s python variant (`step_python_migrate`). Runtime files (address cache, VM logs) go to `$TMPDIR/natron-tart` (`TART_RUN`); SSH control sockets go to `/tmp/natron-tart-$UID` (`TART_SOCK`), because macOS's `$TMPDIR` path is too long for a Unix socket. Only one `natron-sdk.sh`, `natron-build.sh` or `natron-test.sh` runs at a time (lock in that directory).

## Networking

The scripts don't rely on the VM getting an IPv4 address. On hosts with corporate network filters (seen on a macOS 27 host with Cisco Secure Client, CrowdStrike and endpoint filters), the guest gets no DHCP lease and the NAT forwards nothing, even though the vmnet bridge and IPv6 work. So:

- `softwareupdate` ignores `ALL_PROXY`: `provision.sh`'s `clt` step sets the system SOCKS proxy (`networksetup`, on every network service) for the install of the Command Line Tools, and turns it off afterwards (and in the `fixups` step).
- the guest is reached at its IPv4 address when it gets one within a minute, otherwise at its IPv6 link-local address on the vmnet bridge, which is found with `tart exec` (the cirruslabs guest agent). The scripts check that ssh really reaches the expected VM.
- internet access goes through a reverse SOCKS proxy carried by the SSH connection (`ssh -R 127.0.0.1:1080 -o PermitRemoteOpen='*:80 *:443' admin@<address>`; by hand, see section 0 of `INSTALL_MACOS.md`), listening on the guest's loopback only. `provision.sh` passes it as `ALL_PROXY=socks5h://127.0.0.1:1080` to curl, git, MacPorts (`sudo env ALL_PROXY=…`) and osmesa-install. pip can't use SOCKS proxies, so the build step runs `privoxy` on the guest's loopback (HTTP → SOCKS) for it, and QtPy is fetched with curl and installed offline.
- MacPorts syncs its ports tree from `https://distfiles.macports.org/ports.tar.gz` (`port sync`) instead of rsync, which can't use a SOCKS proxy. MacPorts itself is installed and upgraded from the official `.pkg` (`MACPORTS_VERSION` in `provision.sh`).

## Security notes

- The VMs use tart's default shared (NAT) network: they can only be reached from the host, not from the LAN. Don't attach them to a bridged network.
- The SOCKS proxy opens no port on the host, but whatever runs in the guest (port builds, pip, build scripts) can reach, through the host, anything the host can reach on the allowed ports: the internet, but also services on the host's loopback and the intranet or VPN. `GUEST_PROXY_ALLOW` (passed to ssh's `PermitRemoteOpen`) limits the destinations to ports 80 and 443 by default.
- The cirruslabs images have the well-known `admin`/`admin` account with passwordless sudo, and they log in automatically. The scripts use the password once to install a dedicated SSH key, then log in with the key. The account and its password are still in the SDK image.
- Downloads are verified: the MacPorts `.pkg` (checksum and Developer ID signature of the MacPorts release manager), the QtPy wheel (pinned sha256), and the G'MIC sources (sha256 pinned in openfx-gmic's Makefile, checked by its CMake build, which `build-plugins.sh` uses).
- Only files tracked by git under `tools/` are copied from the Natron checkout, and the copies are removed from the image at the end of `create`/`update`. `push` publishes the image, including the default credentials above: check the registry's visibility first.
