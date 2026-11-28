#!/bin/bash
# Helpers of macos_dmg.yml and macos_sdk.yml: the macOS MacPorts SDK of the DMG of one
# architecture (x86_64 or arm64), on GitHub's macOS 15 runners (macos-15-intel, macos-15). The SDK
# is made by tools/tart/provision.sh (the steps of the arm64 tart SDK image, minus disk, nobrew and
# clt), with BUNDLE_ONLY_REBUILD=1; it is saved as a zstd tarball of /opt/local, /opt/osmesa,
# /opt/llvm, /opt/MacPorts-Natron and ~/provision-logs.
#
#   macos_sdk.sh find ARCH KEY MODE  print "name run_id what" of the SDK artifact of ARCH to start
#                                    from (what: reuse, update or scratch; see below)
#   macos_sdk.sh select-xcode        select the Xcode that builds the SDK and the DMG ($XCODE_APP)
#   macos_sdk.sh hide-homebrew       move the runner's Homebrew (/usr/local, /opt/homebrew) out of the way
#   macos_sdk.sh unhide-homebrew     put it back (its zstd and git are used to save the SDK)
#   macos_sdk.sh download NAME RUN DIR   download the SDK or state artifact NAME of run RUN into
#                                    DIR, checked with zstd -t, with retries (needs Homebrew's gh
#                                    and zstd, and GH_TOKEN)
#   macos_sdk.sh restore FILE        extract a saved SDK (needs Homebrew's zstd)
#   macos_sdk.sh run DEADLINE STEP...  run provision.sh STEPs; stop them at DEADLINE (seconds
#                                    since the epoch): exit status 0 if all done, 3 if stopped
#   macos_sdk.sh save FILE [final]   tar the SDK into FILE (final: without the build directories)
#   macos_sdk.sh steps               print the provision.sh steps of the SDK
set -euo pipefail

# the SDK artifact of ARCH is natron-sdk-macos-ARCH-<key> (macos_sdk.yml)
ARTIFACT_PREFIX=natron-sdk-macos-
# The Xcode of the SDK and the DMG: one whose macOS SDK is at most one macOS release above the
# deployment target (NATRON_MACOS_RELEASE_MIN, 14.0), which is the well-supported case (Xcode 16.4,
# macOS 15.5 SDK); the runner (macOS 15) follows the same rule. Both move when the target moves.
XCODE_APP=/Applications/Xcode_16.4.app
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
    # The newest SDK artifacts of ARCH of the earlier runs of this run's workflow (macos_x86_64.yml or
    # macos_arm64.yml; not expired): one of the same KEY (same tools/MacPorts, provision.sh, ...) if
    # it is recent enough, else the newest one of this branch or of the default branch, which is
    # updated (MODE reuse or update); none for MODE scratch.
    arch=$1 key=$2 mode=$3
    case $arch in x86_64|arm64) ;; *) echo "find: ARCH is x86_64 or arm64, not $arch" >&2; exit 2 ;; esac
    prefix=$ARTIFACT_PREFIX$arch-
    [ "$mode" != scratch ] || { echo "- - scratch"; exit 0; }
    : "${GITHUB_REPOSITORY:?}" "${GITHUB_REF_NAME:?}" "${DEFAULT_BRANCH:?}" "${GITHUB_RUN_ID:?}"
    gh_retry() { local _; for _ in 1 2 3; do gh "$@" && return 0; sleep 20; done; return 1; }
    # the workflow of this run (macos_sdk.yml is called by it)
    workflow=$(gh_retry api "repos/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID" --jq .workflow_id)
    list=$(gh_retry api "repos/$GITHUB_REPOSITORY/actions/workflows/$workflow/runs?per_page=40" --jq '.workflow_runs[].id' |
        while read -r id; do
            [ "$id" != "$GITHUB_RUN_ID" ] || continue
            gh_retry api "repos/$GITHUB_REPOSITORY/actions/runs/$id/artifacts?per_page=100" --jq \
                ".artifacts[] | select((.name | startswith(\"$prefix\")) and (.expired | not)) |
                 \"\(.created_at) \(.name) \(.workflow_run.id) \(.workflow_run.head_branch)\""
        done | sort -r)
    echo "SDK artifacts found:" >&2; echo "${list:-(none)}" >&2
    # (runs on Linux: GNU date)
    cutoff=$(date -u -d "$REUSE_MAX_DAYS days ago" +%Y-%m-%dT%H:%M:%SZ)
    if [ "$mode" = reuse ]; then
        hit=$(echo "$list" | awk -v n="$prefix$key" -v c="$cutoff" '$2 == n && $1 >= c {print $2, $3; exit}')
        [ -z "$hit" ] || { echo "$hit reuse"; exit 0; }
    fi
    base=$(echo "$list" | awk -v b="$GITHUB_REF_NAME" -v d="$DEFAULT_BRANCH" '$4 == b || $4 == d {print $2, $3; exit}')
    if [ -n "$base" ]; then echo "$base update"; else echo "- - scratch"; fi
    ;;
select-xcode)
    [ -d "$XCODE_APP" ] || { echo "::error::$XCODE_APP is not on this runner image: pick another Xcode in macos_sdk.sh (XCODE_APP)"; ls -d /Applications/Xcode*.app; exit 1; }
    sudo xcode-select -s "$XCODE_APP/Contents/Developer"
    xcodebuild -version
    xcrun --sdk macosx --show-sdk-version
    ;;
hide-homebrew)
    # Homebrew must be gone, not only off the PATH. On Intel Macs it lives in /usr/local, whose
    # include and lib directories the compilers and the linker search by default; /usr is
    # read-only (signed system volume), so its contents are moved. On arm64 it is /opt/homebrew
    # (and /usr/local has other tools of the runner image), which build systems may still find
    # (e.g. CMake's default prefixes): both are moved.
    hidden=$HOME/usr-local.homebrew
    if [ -d "$hidden" ]; then echo "already hidden"; exit 0; fi
    sudo mkdir "$hidden"
    sudo find /usr/local -mindepth 1 -maxdepth 1 -exec mv {} "$hidden/" \;
    [ -z "$(ls -A /usr/local)" ] || { echo "could not empty /usr/local:"; ls -A /usr/local; exit 1; }
    [ ! -e /opt/homebrew ] || sudo mv /opt/homebrew "$HOME/opt-homebrew.homebrew"
    ;;
unhide-homebrew)
    hidden=$HOME/usr-local.homebrew
    [ ! -e "$HOME/opt-homebrew.homebrew" ] || sudo mv "$HOME/opt-homebrew.homebrew" /opt/homebrew
    [ -d "$hidden" ] || exit 0
    sudo find "$hidden" -mindepth 1 -maxdepth 1 -exec mv {} /usr/local/ \;
    sudo rmdir "$hidden"
    ;;
download)
    # not actions/download-artifact: it once ended "successfully" after 71 MB of a 4.3 GB SDK
    name=$1 run=$2 dir=$3
    for i in 1 2 3; do
        rm -rf "$dir"; mkdir -p "$dir"
        if gh run download "$run" -R "$GITHUB_REPOSITORY" -n "$name" -D "$dir" && zstd -tq "$dir/natron-sdk.tar.zst"; then
            ls -l "$dir"
            exit 0
        fi
        echo "download of $name from run $run failed or is incomplete (attempt $i)"
        sleep 30
    done
    exit 1
    ;;
restore)
    file=$1
    ls -l "$file"
    # the macports user and group (MacPorts builds as them) are created by the MacPorts .pkg: install
    # it first (the version of provision.sh), then the saved /opt/local replaces its files (tar
    # restores the owners by name)
    if ! id macports > /dev/null 2>&1; then
        # provision.sh's step, with its checksum and signature checks (its log and marker are
        # replaced by the saved ones below)
        # (provision.sh reads the checkout at its tart path, ~/Documents/third_party/Natron)
        mkdir -p "$HOME/Documents/third_party"
        ln -sfn "$(cd "$(dirname "$0")/../.." && pwd)" "$HOME/Documents/third_party/Natron"
        "$HOME/Documents/third_party/Natron/tools/tart/provision.sh" macports
        id macports
    fi
    sudo rm -rf /opt/local /opt/osmesa /opt/llvm /opt/MacPorts-Natron "$HOME/provision-logs"
    # -p: keep the owners (macports) and modes
    zstd -dc "$file" | sudo tar -xpf - -C /
    rm -f "$file"
    # A stage's state has no distfiles (see save), but a port build stopped at the deadline keeps
    # its work directory, whose state file says that the distfiles were fetched: MacPorts would
    # then "extract" the missing distfile into nothing (bzip2 fails, tar doesn't) and fail at the
    # patch phase on every retry (libassuan in run 46020996105). Forget the fetch and checksum of
    # those builds, so that MacPorts fetches their distfiles again; the later phases stay done.
    # (tee rewrites the file in place: it stays owned by, and writable for, the macports user)
    states=/opt/local/var/macports/build
    if [ -d "$states" ]; then
        sudo find "$states" -maxdepth 3 -path '*/work/.macports.*.state' -print0 |
            while IFS= read -r -d '' f; do
                sudo grep -qE '^target: org\.macports\.(fetch|checksum)$' "$f" || continue
                # fetch.type git or hg (libvpx, ctl): the fetch is a clone into the work directory,
                # which the state has (and a second clone into it would fail)
                [ -z "$(sudo find "$(dirname "$f")" -mindepth 2 -maxdepth 2 \( -name .git -o -name .hg \))" ] || continue
                echo "resumed build, its distfiles will be fetched again: $f"
                kept=$(sudo grep -vE '^target: org\.macports\.(fetch|checksum)$' "$f")
                printf '%s\n' "$kept" | sudo tee "$f" > /dev/null
            done
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
    sed -n 2,20p "$0"; exit 2 ;;
esac
