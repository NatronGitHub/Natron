#!/bin/bash
# Build Natron (plugins + app + DMG) in a throw-away clone of the Natron SDK tart image.
#
#   natron-build.sh [options]
#     -b BRANCH   Natron branch to build (default: RB-2.6)
#     -t          also run the unit/regression tests (UNIT_TESTS=true)
#     -o DIR      where to copy the results (default: ./natron-builds/<VM name>)
#     -k          keep the build VM afterwards (default: delete it)
#     -i IMAGE    SDK image to clone (default: $SDK_IMAGE, see tart-common.sh)
#     -s SYSTEM   build system for Natron: cmake (default) or qmake (deprecated)
#     -l LICENSE  NATRON_LICENSE: GPL (default) or COMMERCIAL (LGPL FFmpeg, LibRaw and libheif,
#                 no GPL component: checked by tools/jenkins/check-commercial-bundle.sh)
#
# NATRON_EXTRA_CMAKE_FLAGS in the environment is passed on to the CMake build.
# The local tools/jenkins scripts and tools/MacPorts overlay are used (copied into the VM);
# the Natron and plugin sources are checked out from GitHub by launchBuildMain.sh.
# The build fails if a Mach-O file in the DMG needs a macOS newer than $MACOS_MIN (NATRON_MACOS_RELEASE_MIN in tools/jenkins/macos-release.sh),
# see check-minos.sh; the list is in <dmg name>-minos.txt next to the DMG.
set -euo pipefail
. "$(dirname "$0")/tart-common.sh"

BRANCH=RB-2.6
UNIT_TESTS=false
OUT=
KEEP=0
BUILD_SYSTEM=cmake
LICENSE=GPL
while getopts "b:to:ki:s:l:h" opt; do
    case $opt in
        b) BRANCH=$OPTARG ;;
        t) UNIT_TESTS=true ;;
        o) OUT=$OPTARG ;;
        k) KEEP=1 ;;
        i) SDK_IMAGE=$OPTARG ;;
        s) BUILD_SYSTEM=$OPTARG ;;
        l) LICENSE=$OPTARG ;;
        *) sed -n 2,18p "$0"; exit 1 ;;
    esac
done
case "$LICENSE" in
    GPL|COMMERCIAL) ;;
    *) echo "-l: GPL or COMMERCIAL, not $LICENSE"; exit 1 ;;
esac

tart_lock
trap tart_unlock EXIT   # replaced by cleanup once the clone exists
vm_exists "$SDK_IMAGE" || { echo "SDK image $SDK_IMAGE not found: run natron-sdk.sh create (or tart pull it)"; exit 1; }
VM=natron-build-$(date +%Y%m%d-%H%M%S)-$$
OUT=${OUT:-$PWD/natron-builds/$VM}

cleanup() {
    vm_stop "$VM" || true
    if [ "$KEEP" = 0 ]; then tart delete "$VM" || log "Warning: could not delete $VM"; else log "kept VM $VM"; fi
    tart_unlock
}

log "cloning $SDK_IMAGE into $VM"
tart clone "$SDK_IMAGE" "$VM"
trap cleanup EXIT   # only once the clone (ours) exists
vm_configure "$VM"
vm_start "$VM"
vm_sync "$VM"
status=0
vm_provision "$VM" GIT_BRANCH="$BRANCH" UNIT_TESTS="$UNIT_TESTS" NATRON_BUILD_SYSTEM="$BUILD_SYSTEM" NATRON_LICENSE="$LICENSE" \
    NATRON_EXTRA_CMAKE_FLAGS="${NATRON_EXTRA_CMAKE_FLAGS:-}" -- sync build || status=$?

log "copying results to $OUT"
mkdir -p "$OUT"
vm_rsync_back "$VM" provision-logs/ "$OUT/logs/" || true
vm_rsync_back "$VM" Development/workspace/builds_archive/ "$OUT/" || true
ls -lR "$OUT" | grep -E '\.dmg|tests\.txt|failures\.zip' || true
if [ "$status" = 0 ] && ! ls "$OUT"/*/*/*.dmg > /dev/null 2>&1; then
    log "Error: the build succeeded but no DMG was copied to $OUT: keeping $VM"
    status=1; KEEP=1
fi
# Every Mach-O file in the DMG must run on the oldest supported macOS ($MACOS_MIN, default NATRON_MACOS_RELEASE_MIN,
# tools/jenkins/macos-release.sh): a release fails otherwise.
MACOS_MIN=${MACOS_MIN:-$NATRON_MACOS_RELEASE_MIN}
for dmg in "$OUT"/*/*/*.dmg; do
    [ -f "$dmg" ] || continue
    log "checking that every Mach-O file in $(basename "$dmg") runs on macOS $MACOS_MIN"
    if "$(dirname "$0")/check-minos.sh" -t "$MACOS_MIN" "$dmg" > "${dmg%.dmg}-minos.txt"; then
        log "minos check OK"
    else
        log "Error: files in $(basename "$dmg") need a macOS newer than $MACOS_MIN (${dmg%.dmg}-minos.txt):"
        head -20 "${dmg%.dmg}-minos.txt"
        status=1
    fi
done
[ "$status" = 0 ] && log "build OK" || log "build FAILED (status $status), see $OUT/logs/build.log"
exit "$status"
