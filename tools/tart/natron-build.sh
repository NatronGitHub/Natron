#!/bin/bash
# Build Natron (plugins + app + DMG) in a throw-away clone of the Natron SDK tart image.
#
#   natron-build.sh [options]
#     -b BRANCH   Natron branch to build (default: RB-2.6)
#     -t          also run the unit/regression tests (UNIT_TESTS=true)
#     -o DIR      where to copy the results (default: ./natron-builds/<VM name>)
#     -k          keep the build VM afterwards (default: delete it)
#     -i IMAGE    SDK image to clone (default: $SDK_IMAGE, see tart-common.sh)
#
# The local tools/jenkins scripts and tools/MacPorts overlay are used (copied into the VM);
# the Natron and plugin sources are checked out from GitHub by launchBuildMain.sh.
set -euo pipefail
. "$(dirname "$0")/tart-common.sh"

BRANCH=RB-2.6
UNIT_TESTS=false
OUT=
KEEP=0
while getopts "b:to:ki:h" opt; do
    case $opt in
        b) BRANCH=$OPTARG ;;
        t) UNIT_TESTS=true ;;
        o) OUT=$OPTARG ;;
        k) KEEP=1 ;;
        i) SDK_IMAGE=$OPTARG ;;
        *) sed -n 2,12p "$0"; exit 1 ;;
    esac
done

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
vm_provision "$VM" GIT_BRANCH="$BRANCH" UNIT_TESTS="$UNIT_TESTS" -- sync build || status=$?

log "copying results to $OUT"
mkdir -p "$OUT"
vm_rsync_back "$VM" provision-logs/ "$OUT/logs/" || true
vm_rsync_back "$VM" Development/workspace/builds_archive/ "$OUT/" || true
ls -lR "$OUT" | grep -E '\.dmg|tests\.txt|failures\.zip' || true
if [ "$status" = 0 ] && ! ls "$OUT"/*/*/*.dmg > /dev/null 2>&1; then
    log "Error: the build succeeded but no DMG was copied to $OUT: keeping $VM"
    status=1; KEEP=1
fi
[ "$status" = 0 ] && log "build OK" || log "build FAILED (status $status), see $OUT/logs/build.log"
exit "$status"
