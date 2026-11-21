#!/bin/bash
# Build or update the local tart image containing the Natron macOS SDK
# (MacPorts + Natron overlay ports, OSMesa/LLVM, clang, Qt5/PySide2, packaging tools).
#
#   natron-sdk.sh create    build $SDK_IMAGE from $BASE_IMAGE (resumes an interrupted run;
#                           REDO="step..." re-runs steps already done in $SDK_IMAGE-wip)
#   natron-sdk.sh update [STEP...]  refresh an existing $SDK_IMAGE (port sync, overlay, pinned ports,
#                           upgrade outdated); STEPs already done in the image are re-run (e.g. python_qt)
#   natron-sdk.sh push REF  push $SDK_IMAGE to an OCI registry (e.g. ghcr.io/<org>/natron-sdk-sonoma:2026-11)
#
# Work happens on a copy ($SDK_IMAGE-wip); $SDK_IMAGE is only replaced when everything succeeded,
# and the previous image is kept as $SDK_IMAGE-prev.
# Logs inside the VM: ~admin/provision-logs/<step>.log
set -euo pipefail
. "$(dirname "$0")/tart-common.sh"

WIP=$SDK_IMAGE-wip

# on failure the -wip VM is left running so that "create" can resume it; close the SSH master
tart_lock
trap 'ssh $(tart_sshopts "$WIP") -O exit admin@dummy 2>/dev/null || true; tart_unlock' EXIT

finish() { # replace $SDK_IMAGE with $WIP, keep the old one as -prev
    vm_provision "$WIP" -- cleanup
    vm_stop "$WIP"
    if vm_exists "$SDK_IMAGE"; then
        vm_exists "$SDK_IMAGE-prev" && tart delete "$SDK_IMAGE-prev"
        tart rename "$SDK_IMAGE" "$SDK_IMAGE-prev"
    fi
    tart rename "$WIP" "$SDK_IMAGE"
    log "$SDK_IMAGE is ready"
    tart list
}

case "${1:-}" in
create)
    if ! vm_exists "$WIP"; then
        log "cloning $BASE_IMAGE into $WIP (the first pull downloads about 30 GB)"
        tart clone "$BASE_IMAGE" "$WIP"
    else
        log "resuming with existing $WIP"
    fi
    vm_configure "$WIP"
    vm_start "$WIP"
    vm_sync "$WIP"
    vm_provision "$WIP" REDO="${REDO:-}"   # all SDK steps; finished steps are skipped unless in $REDO
    finish
    ;;
update)
    vm_exists "$SDK_IMAGE" || { echo "$SDK_IMAGE does not exist, run '$0 create' first"; exit 1; }
    vm_exists "$WIP" && { echo "$WIP exists (interrupted run?): resume it with '$0 create' (an interrupted update is finished first; REDO=\"step...\" re-runs done steps), or delete it with 'tart stop $WIP; tart delete $WIP'"; exit 1; }
    tart clone "$SDK_IMAGE" "$WIP"
    vm_configure "$WIP"
    vm_start "$WIP"
    vm_sync "$WIP"
    shift
    vm_provision "$WIP" -- update
    vm_provision "$WIP" REDO="$*"  # runs SDK steps not yet in the image (new steps) and the given STEPs;
                                   # a port added to an existing step needs that step named here
    finish
    ;;
push)
    [ -n "${2:-}" ] || { echo "usage: $0 push REGISTRY/IMAGE:TAG"; exit 1; }
    tart push "$SDK_IMAGE" "$2"
    ;;
*)
    sed -n 2,13p "$0"; exit 1 ;;
esac
