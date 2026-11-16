# Automated macOS builds with tart

These scripts build Natron for macOS in [tart](https://tart.run) virtual machines on an Apple Silicon Mac. They are the macOS counterpart of `tools/docker`:

| | Docker (Linux) | tart (macOS) |
|---|---|---|
| SDK image (all build dependencies) | `natron-sdk` | `natron-sdk.sh` → local image `natron-sdk-sequoia` |
| build | `natron-build` | `natron-build.sh` |

The baseline for distribution is **macOS 15 Sequoia** (base image `ghcr.io/cirruslabs/macos-sequoia-xcode:latest`). The SDK is installed with the same procedure as the MacPorts section of `INSTALL_MACOS.md`: Xcode, MacPorts + the `tools/MacPorts` overlay, OSMesa/LLVM via [osmesa-install](https://github.com/devernay/osmesa-install), the MacPorts clang matching the OS, libraries, Qt5/PySide2, and the DMG tools.

## Requirements

**The host must be a Mac with Apple Silicon.** tart is a thin layer over Apple's Virtualization.framework, so it does not run on Intel Macs, Linux or Windows. The guests it can run are macOS 12 or later (arm64) and arm64 Linux. That means:

- These scripts only produce **arm64 macOS** builds. Intel/universal macOS builds, and builds or tests on macOS 11 or older (e.g. 10.15), need an Intel Mac or a VM on one (see the notes for older systems in `INSTALL_MACOS.md`).
- Linux and Windows releases are not built here: use `tools/docker` (Linux) and the MSYS2 builds (Windows).

Other requirements:

- `tart` (`brew install cirruslabs/cli/tart`).
- rsync 3 (`brew install rsync` or `sudo port install rsync`). macOS's own `/usr/bin/rsync` (openrsync) can't copy to IPv6 addresses.
- `sshpass`, used once to install the SSH key (`brew install hudochenkov/sshpass/sshpass`).
- About 70 GB to download the base image the first time, plus up to `VM_DISK` (250 GB, sparse) per VM.
- The Natron checkout containing these scripts. Optionally, `../osmesa-install` next to it (its working copy is used); otherwise it is cloned from GitHub inside the VM.

## Build or update the SDK image

```sh
tools/tart/natron-sdk.sh create          # from the cirruslabs base image; resumable; takes hours
tools/tart/natron-sdk.sh update [STEP…]  # MacPorts pkg if MACPORTS_VERSION changed, port sync, overlay,
                                         # pinned ports, upgrade outdated ports; STEPs are re-run
tools/tart/natron-sdk.sh push ghcr.io/<org>/natron-sdk-sequoia:<tag>   # optional, to share it
```

Both `create` and `update` work on a copy named `natron-sdk-sequoia-wip`. The image is replaced only when every step succeeded, and the previous image is kept as `natron-sdk-sequoia-prev`.

If a step fails, the `-wip` VM is left running. Fix the cause on the host (for example in `tools/MacPorts`), then resume with `natron-sdk.sh create`, for both `create` and `update` (an interrupted update is finished first): steps that already succeeded are skipped, and `REDO="step…"` re-runs some of them. `update` re-runs only new steps and the steps you name: after adding a port to an existing step (e.g. `libs`), run `natron-sdk.sh update libs`. The logs are in `~admin/provision-logs/<step>.log` inside the VM. To look at them, source `tart-common.sh` and run `vm_ssh natron-sdk-sequoia-wip`.

Before `update`, reintegrate upstream MacPorts changes into the overlay on the host (`tools/MacPorts/check.sh`, see `tools/MacPorts/skills/macports-natron-upgrade/SKILL.md`). The `update` step only warns when the overlay is out of date.

## Build Natron

```sh
tools/tart/natron-build.sh                 # RB-2.6, release, no tests
tools/tart/natron-build.sh -b RB-2.6 -t    # also run the unit/regression tests
```

Each build runs in a fresh clone of the SDK image. Clones are copy-on-write, so they are fast and small. The clone is deleted afterwards (`-k` keeps it). The results (`.dmg`, test results, logs) are copied to `./natron-builds/<vm-name>/`.

The files tracked by git under `tools/` are copied into the VM with their local changes, so uncommitted changes to `tools/jenkins` are tested. Changes to the `tools/MacPorts` overlay are published (`portindex`) but installed ports are not rebuilt: run `natron-sdk.sh update` for that. The Natron and plugin sources are checked out from GitHub by `launchBuildMain.sh`.

Only one Natron VM can run at a time: clones of an image share their IPv6 link-local address, so the scripts refuse to start a VM while another `natron-sdk-*` or `natron-build-*` VM is running.

## Settings

Environment variables (see `tart-common.sh`): `BASE_IMAGE`, `SDK_IMAGE`, `VM_CPU` (8), `VM_MEMORY` (16384 MB), `VM_DISK` (250 GB), `TART_KEY` (`~/.ssh/natron-tart_ed25519`), `OSMESA_INSTALL_SRC`, `GUEST_PROXY_ALLOW` (`*:80 *:443`), and `RSYNC`. `MACPORTS_VERSION` is at the top of `provision.sh`; the Python version (3.10) is spelled out in `step_python_qt` and `step_fixups`. Runtime files (address cache, VM logs) go to `$TMPDIR/natron-tart` (`TART_RUN`); SSH control sockets go to `/tmp/natron-tart-$UID` (`TART_SOCK`), because macOS's `$TMPDIR` path is too long for a Unix socket. Only one `natron-sdk.sh` or `natron-build.sh` runs at a time (lock in that directory).

## Networking

The scripts don't rely on the VM getting an IPv4 address. On hosts with corporate network filters (seen on a macOS 27 host with Cisco Secure Client, CrowdStrike and endpoint filters), the guest gets no DHCP lease and the NAT forwards nothing, even though the vmnet bridge and IPv6 work. So:

- the guest is reached at its IPv4 address when it gets one within a minute, otherwise at its IPv6 link-local address on the vmnet bridge, which is found with `tart exec` (the cirruslabs guest agent). The scripts check that ssh really reaches the expected VM.
- internet access goes through a reverse SOCKS proxy carried by the SSH connection (`ssh -R 127.0.0.1:1080`), listening on the guest's loopback only. `provision.sh` passes it as `ALL_PROXY=socks5h://127.0.0.1:1080` to curl, git, MacPorts (`sudo env ALL_PROXY=…`) and osmesa-install. pip can't use SOCKS proxies, so the build step runs `privoxy` on the guest's loopback (HTTP → SOCKS) for it, and QtPy is fetched with curl and installed offline.
- MacPorts syncs its ports tree from `https://distfiles.macports.org/ports.tar.gz` (`port sync`) instead of rsync, which can't use a SOCKS proxy. MacPorts itself is installed and upgraded from the official `.pkg` (`MACPORTS_VERSION` in `provision.sh`).

## Security notes

- The VMs use tart's default shared (NAT) network: they can only be reached from the host, not from the LAN. Don't attach them to a bridged network.
- The SOCKS proxy opens no port on the host, but whatever runs in the guest (port builds, pip, build scripts) can reach, through the host, anything the host can reach on the allowed ports: the internet, but also services on the host's loopback and the intranet or VPN. `GUEST_PROXY_ALLOW` (passed to ssh's `PermitRemoteOpen`) limits the destinations to ports 80 and 443 by default.
- The cirruslabs images have the well-known `admin`/`admin` account with passwordless sudo. The scripts use the password once to install a dedicated SSH key, then log in with the key. The account and its password are still in the SDK image.
- Downloads are verified: the MacPorts `.pkg` (checksum and Developer ID signature of the MacPorts release manager), the QtPy wheel (pinned sha256), and, in `build-plugins.sh`, the G'MIC sources (pinned sha256).
- Only files tracked by git under `tools/` are copied from the Natron checkout, and the copies are removed from the image at the end of `create`/`update`. `push` publishes the image, including the default credentials above: check the registry's visibility first.
