#!/bin/bash
# Helpers of macos_x86_64.yml and macos_x86_64_sdk.yml: the macOS x86_64 MacPorts SDK on GitHub's
# macos-15-intel runners. The SDK is made by tools/tart/provision.sh (the steps of the arm64 tart
# SDK image, minus disk, nobrew and clt), with BUNDLE_ONLY_REBUILD=1; it is saved as a zstd tarball
# of /opt/local, /opt/osmesa, /opt/llvm, /opt/MacPorts-Natron and ~/provision-logs.
#
#   macos_x86_64_sdk.sh find KEY MODE    print "name run_id what" of the SDK artifact to start from
#                                        (what: reuse, update or scratch; see below)
#   macos_x86_64_sdk.sh hide-homebrew    move the runner's Homebrew (/usr/local) out of the way
#   macos_x86_64_sdk.sh unhide-homebrew  put it back (its zstd and git are used to save the SDK)
#   macos_x86_64_sdk.sh restore FILE     extract a saved SDK (needs Homebrew's zstd)
#   macos_x86_64_sdk.sh run DEADLINE STEP...  run provision.sh STEPs; stop them at DEADLINE (seconds
#                                        since the epoch): exit status 0 if all done, 3 if stopped
#   macos_x86_64_sdk.sh save FILE [final]     tar the SDK into FILE (final: without the build directories)
set -euo pipefail

ARTIFACT_PREFIX=natron-sdk-macos-x86_64-
# SDK steps of provision.sh on a runner: not disk, nobrew (Homebrew is hidden instead) and clt (the
# runner's Xcode is used: GitHub has no macOS 14 runner, and no Command Line Tools 15.3)
SDK_STEPS="macports macports_conf overlay sync pins osmesa compiler tools libs python_qt dmgtools rebuild_target fixups"
# reuse an SDK of the same definition only if it is less than this old: older ones are updated, so
# that the SDK in use is refreshed (the artifacts expire after 90 days)
REUSE_MAX_DAYS=30
PATHS=(opt/local opt/osmesa opt/llvm opt/MacPorts-Natron "${HOME#/}/provision-logs")

cmd=${1:-}; shift || true
case $cmd in
find)
    # The newest SDK artifacts of this workflow's earlier runs (not expired): one of the same KEY
    # (same tools/MacPorts, provision.sh, ...) if it is recent enough, else the newest one of this
    # branch or of the default branch, which is updated (MODE reuse or update); none for MODE scratch.
    key=$1 mode=$2
    [ "$mode" != scratch ] || { echo "- - scratch"; exit 0; }
    : "${GITHUB_REPOSITORY:?}" "${GITHUB_REF_NAME:?}" "${DEFAULT_BRANCH:?}" "${GITHUB_RUN_ID:?}"
    gh_retry() { local _; for _ in 1 2 3; do gh "$@" && return 0; sleep 20; done; return 1; }
    list=$(gh_retry api "repos/$GITHUB_REPOSITORY/actions/workflows/macos_x86_64.yml/runs?per_page=40" --jq '.workflow_runs[].id' |
        while read -r id; do
            [ "$id" != "$GITHUB_RUN_ID" ] || continue
            gh_retry api "repos/$GITHUB_REPOSITORY/actions/runs/$id/artifacts?per_page=100" --jq \
                ".artifacts[] | select((.name | startswith(\"$ARTIFACT_PREFIX\")) and (.expired | not)) |
                 \"\(.created_at) \(.name) \(.workflow_run.id) \(.workflow_run.head_branch)\""
        done | sort -r)
    echo "SDK artifacts found:" >&2; echo "${list:-(none)}" >&2
    cutoff=$(date -u -v-${REUSE_MAX_DAYS}d +%Y-%m-%dT%H:%M:%SZ)
    if [ "$mode" = reuse ]; then
        hit=$(echo "$list" | awk -v n="$ARTIFACT_PREFIX$key" -v c="$cutoff" '$2 == n && $1 >= c {print $2, $3; exit}')
        [ -z "$hit" ] || { echo "$hit reuse"; exit 0; }
    fi
    base=$(echo "$list" | awk -v b="$GITHUB_REF_NAME" -v d="$DEFAULT_BRANCH" '$4 == b || $4 == d {print $2, $3; exit}')
    if [ -n "$base" ]; then echo "$base update"; else echo "- - scratch"; fi
    ;;
hide-homebrew)
    # On Intel Macs Homebrew lives in /usr/local, whose include and lib directories the compilers
    # and the linker search by default: it must be gone, not only off the PATH. /usr is read-only
    # (signed system volume), so its contents are moved.
    hidden=$HOME/usr-local.homebrew
    if [ -d "$hidden" ]; then echo "already hidden"; exit 0; fi
    sudo mkdir "$hidden"
    sudo find /usr/local -mindepth 1 -maxdepth 1 -exec mv {} "$hidden/" \;
    [ -z "$(ls -A /usr/local)" ] || { echo "could not empty /usr/local:"; ls -A /usr/local; exit 1; }
    ;;
unhide-homebrew)
    hidden=$HOME/usr-local.homebrew
    [ -d "$hidden" ] || exit 0
    sudo find "$hidden" -mindepth 1 -maxdepth 1 -exec mv {} /usr/local/ \;
    sudo rmdir "$hidden"
    ;;
restore)
    file=$1
    ls -l "$file"
    sudo rm -rf /opt/local /opt/osmesa /opt/llvm /opt/MacPorts-Natron "$HOME/provision-logs"
    # -p: keep the owners (macports) and modes
    zstd -dc "$file" | sudo tar -xpf - -C /
    rm -f "$file"
    # the macports user (MacPorts builds as it) exists only where the MacPorts .pkg was installed:
    # recreate it, with the uid that owns its files, as the .pkg's postinstall does
    if ! id macports > /dev/null 2>&1; then
        uid=$(stat -f %u /opt/local/var/macports/home)
        gid=$(stat -f %g /opt/local/var/macports/home)
        sudo dscl . -create /Groups/macports PrimaryGroupID "$gid"
        sudo dscl . -create /Groups/macports Password '*'
        sudo dscl . -create /Users/macports UniqueID "$uid"
        sudo dscl . -create /Users/macports PrimaryGroupID "$gid"
        sudo dscl . -create /Users/macports NFSHomeDirectory /opt/local/var/macports/home
        sudo dscl . -create /Users/macports UserShell /usr/bin/false
        sudo dscl . -create /Users/macports RealName MacPorts
        sudo dscl . -create /Users/macports Password '*'
        id macports
    fi
    sudo chown -R "$(id -un)" "$HOME/provision-logs"
    cat /opt/local/etc/natron-sdk.txt 2> /dev/null || true
    df -h /
    ;;
run)
    deadline=$1; shift
    steps=$*
    # an update interrupted by the deadline of the previous stage is finished first
    if [ -f "$HOME/provision-logs/update.pending" ] && [[ " $steps " != *" update "* ]]; then steps="update $steps"; fi
    echo "provision.sh $steps, until $(date -r "$deadline" '+%F %T')"
    # in its own process group (perl's setpgrp), to stop all of it at the deadline
    # shellcheck disable=SC2086 # one word per step
    BUNDLE_ONLY_REBUILD=1 perl -e 'setpgrp(0, 0); exec @ARGV' "$HOME/Documents/third_party/Natron/tools/tart/provision.sh" $steps &
    pid=$!
    stopped=0
    while kill -0 "$pid" 2> /dev/null; do
        if [ "$(date +%s)" -ge "$deadline" ]; then
            echo "::notice::time budget of this stage reached: stopping provision.sh, the next stage resumes it"
            stopped=1
            # port runs as root through sudo: signal the whole group as root, then whatever is left
            sudo kill -TERM -- "-$pid" 2> /dev/null || true
            sleep 30
            sudo kill -KILL -- "-$pid" 2> /dev/null || true
            sudo pkill -KILL -f '/opt/local/bin/port' 2> /dev/null || true
            sudo pkill -KILL -f '/opt/local/var/macports/build/' 2> /dev/null || true
            # the compilers that MacPorts' builds started (nothing else builds on this runner)
            sudo pkill -KILL -x 'g?make|ninja|clang(\+\+)?|ld' 2> /dev/null || true
            sleep 5
            ps -axo pid,pgid,user,command | tail -40   # what is left running
            break
        fi
        sleep 20
    done
    rc=0; wait "$pid" || rc=$?
    if [ "$stopped" = 1 ]; then
        # the step that was running keeps no done marker: it is re-run (MacPorts resumes its build)
        exit 3
    fi
    exit "$rc"
    ;;
save)
    file=$1 final=${2:-}
    excludes=(--exclude opt/local/var/macports/distfiles --exclude opt/local/var/macports/incoming)
    if [ "$final" = final ]; then
        excludes+=(--exclude 'opt/local/var/macports/build/*')
        printf 'key %s\ncommit %s\ndate %s\nrun %s/%s/actions/runs/%s\n' "${SDK_KEY_SHORT:-}" "${GITHUB_SHA:-}" "$(date -u '+%F %T')" \
            "${GITHUB_SERVER_URL:-}" "${GITHUB_REPOSITORY:-}" "${GITHUB_RUN_ID:-}" | sudo tee /opt/local/etc/natron-sdk.txt
    fi
    mkdir -p "$(dirname "$file")"
    du -sh /opt/local 2> /dev/null || true
    paths=(); for p in "${PATHS[@]}"; do [ ! -e "/$p" ] || paths+=("$p"); done
    sudo tar -cf - "${excludes[@]}" -C / "${paths[@]}" | zstd -T0 -3 -q -o "$file"
    ls -l "$file"
    df -h /
    ;;
steps)
    echo "$SDK_STEPS"
    ;;
*)
    sed -n 2,14p "$0"; exit 2 ;;
esac
