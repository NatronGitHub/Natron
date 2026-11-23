#!/bin/bash
# Run the Natron-Tests regression suite (runTests.sh) against a Natron.app, on the machine this
# script runs on, and collect the results as in the Natron release assets. The machine should be a
# bare macOS: no MacPorts, Homebrew or Natron SDK on the PATH or in /opt/local, so that a missing
# or too-new bundled library shows up. Used inside the test VM by natron-test.sh, and on GitHub's
# runners by .github/workflows/macos_x86_64.yml.
#
#   run-natron-tests.sh NATRON.app|NATRON.dmg NATRON_TESTS_DIR OUTDIR NAME
#
# NATRON_TESTS_DIR is a Natron-Tests checkout with known_failures/ (its result.txt, summary.txt and
# failed/ are overwritten). TEST=TestFoo or TESTCUSTOM=TestBar run a single test (see runTests.sh);
# TEST_TIMEOUT (seconds, default 14400) limits the whole suite; ALL_PROXY is passed on to runTests.sh.
# Results in OUTDIR:
#   NAME-tests.txt                  runTests.sh's result.txt (one "Test : PASS", "FAIL",
#                                   "KNOWN FAIL" or "UNEXPECTED PASS" line per test)
#   NAME-tests-summary.txt          the counts of PASS, FAIL, KNOWN FAIL, UNEXPECTED PASS
#   NAME-unit_tests_failures.zip    the failed tests' outputs and references, in a
#                                   NAME-unit_tests_failures/ directory (only if a test failed)
#   NAME-tests.log                  runTests.sh's output
# Exit status: 0 if every test passed or is a known failure, 1 otherwise (or if the suite did not
# run). Unexpected passes are reported, and don't fail the run.
set -euo pipefail

[ $# = 4 ] || { sed -n 2,23p "$0"; exit 2; }
SRC=$1
TESTS=$(cd "$2" && pwd)
mkdir -p "$3"
OUT=$(cd "$3" && pwd)
NAME=$4
TEST_TIMEOUT=${TEST_TIMEOUT:-14400}
log() { echo "=== $(date '+%F %T') $*"; }

[ -f "$TESTS/known_failures/common.txt" ] ||
    { echo "$TESTS has no known_failures/common.txt: use a Natron-Tests with the known-failures lists" >&2; exit 2; }
rm -f "$OUT/$NAME-tests.log" "$OUT/$NAME-tests.txt" "$OUT/$NAME-tests-summary.txt" "$OUT/$NAME-unit_tests_failures.zip"
work=$(mktemp -d "${TMPDIR:-/tmp}/natron-tests.XXXXXX")
mnt=
cleanup() {
    [ -z "$mnt" ] || hdiutil detach -quiet "$mnt" || hdiutil detach -quiet -force "$mnt" || true
    # the app copied from the DMG has read-only directories
    chmod -R u+w "$work" 2> /dev/null || true
    rm -rf "$work"
}
trap cleanup EXIT

# the app, copied out of the DMG (read-only, and its path must not change during the tests)
if [ "${SRC%.dmg}" != "$SRC" ]; then
    mnt=$work/dmg; mkdir "$mnt"
    hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$mnt" "$SRC" > /dev/null
    app=$(ls -d "$mnt"/*.app | head -1)
    ditto "$app" "$work/Natron.app"
    hdiutil detach -quiet "$mnt"; mnt=
    APP=$work/Natron.app
else
    APP=$(cd "$SRC" && pwd)
fi
[ -x "$APP/Contents/MacOS/NatronRenderer" ] || { echo "no NatronRenderer in $APP" >&2; exit 2; }

# gtimeout (GNU coreutils) is what runTests.sh uses on macOS: a minimal one in Perl (part of macOS)
mkdir -p "$work/bin"
cat > "$work/bin/gtimeout" <<"EOF"
#!/usr/bin/perl
# minimal GNU timeout: gtimeout [-s SIGNAL] DURATION COMMAND [ARG...]; exit status 124 on timeout
# the command runs in its own process group, which is killed as a whole on timeout
use strict; use POSIX ":sys_wait_h";
my $sig = "TERM";
while (@ARGV && $ARGV[0] =~ /^-/) { my $o = shift @ARGV; if ($o eq "-s") { $sig = shift @ARGV } elsif ($o =~ /^-s(.+)/) { $sig = $1 } elsif ($o eq "--") { last } }
my $d = shift @ARGV; $d =~ s/s$//; $d *= 60 if $d =~ s/m$//; $d *= 3600 if $d =~ s/h$//;
$sig =~ s/^SIG//;
my $pid = fork(); die "fork: $!" unless defined $pid;
if ($pid == 0) { setpgrp(0, 0); exec { $ARGV[0] } @ARGV or die "exec $ARGV[0]: $!"; }
my $timedout = 0;
local $SIG{ALRM} = sub { $timedout = 1; kill $sig, -$pid; };
alarm $d;
waitpid($pid, 0); my $st = $?; alarm 0;
exit 124 if $timedout;
exit($st & 127 ? 128 + ($st & 127) : $st >> 8);
EOF
chmod +x "$work/bin/gtimeout"

log "running Natron-Tests on $APP (TEST=${TEST:-${TESTCUSTOM:-all}})"
rm -rf "$TESTS/result.txt" "$TESTS/summary.txt" "$TESTS/failed"
rm -rf "$HOME/Library/Caches/INRIA/Natron" && mkdir -p "$HOME/Library/Caches/INRIA/Natron/"{ViewerCache,DiskCache}
status=0
(cd "$TESTS" &&
    env -i HOME="$HOME" USER="$USER" LOGNAME="$USER" SHELL=/bin/bash TMPDIR="${TMPDIR:-/tmp}" LANG=en_US.UTF-8 \
        PATH="$work/bin:/usr/bin:/bin:/usr/sbin:/sbin" \
        ${ALL_PROXY:+ALL_PROXY=$ALL_PROXY NO_PROXY=localhost,127.0.0.1} \
        ${TEST:+TEST=$TEST} ${TESTCUSTOM:+TESTCUSTOM=$TESTCUSTOM} \
        NATRON_DISK_CACHE_PATH="$work/NatronTmpCacheDir" \
        OCIO="$APP/Contents/Resources/OpenColorIO-Configs/blender/config.ocio" \
        FFMPEG="$APP/Contents/MacOS/ffmpeg" COMPARE="$APP/Contents/MacOS/idiff" \
        "$work/bin/gtimeout" -s KILL "$TEST_TIMEOUT" /bin/bash runTests.sh "$APP/Contents/MacOS/NatronRenderer" \
        > "$OUT/$NAME-tests.log" 2>&1) || status=$?
log "runTests.sh exited with status $status"

# runTests.sh only copies the images of the failed render tests to failed/: also copy those
# of the failed custom tests (TestCMD, TestWriteFFmpeg/<format>, ...), as <test>-<name>
if [ -f "$TESTS/result.txt" ]; then
    mkdir -p "$TESTS/failed"
    (cd "$TESTS" && grep -E " : (KNOWN )?FAIL\$" result.txt | sed -E "s/ : (KNOWN )?FAIL\$//" | while IFS= read -r t; do
        case $t in */*)
            d=${t%%/*}/formats/${t#*/}
            if [ -d "$d" ]; then   # TestWriteFFmpeg/<format>: all the frames of that format
                for f in "$d"/reference*.jpg "$d"/output*.jpg "$d"/comp*.jpg; do
                    [ -f "$f" ] && cp "$f" "failed/$(echo "$t" | tr / -)-$(basename "$f")"
                done
            else                   # TestCMD/<n>: frame n of the test
                d=${t%/*}; n=${t##*/}
                for f in "$d/reference$n.jpg" "$d/output$n.jpg" "$d/comp$n.jpg"; do
                    [ -f "$f" ] && cp "$f" "failed/$(echo "$d" | tr / -)-$(basename "$f")"
                done
            fi ;;
        esac
    done) || true
    cp "$TESTS/result.txt" "$OUT/$NAME-tests.txt"
    [ ! -f "$TESTS/summary.txt" ] || cp "$TESTS/summary.txt" "$OUT/$NAME-tests-summary.txt"
    if [ -n "$(ls -A "$TESTS/failed")" ]; then
        mkdir "$work/$NAME-unit_tests_failures"
        cp -R "$TESTS/failed/" "$work/$NAME-unit_tests_failures/"
        (cd "$work" && zip -r -q "$OUT/$NAME-unit_tests_failures.zip" "$NAME-unit_tests_failures")
    fi
fi

if [ -f "$OUT/$NAME-tests.txt" ] && [ -f "$OUT/$NAME-tests-summary.txt" ]; then
    # runTests.sh exits 1 when a test fails that isn't a known failure; summary.txt has the counts
    log "Natron-Tests ($OUT/$NAME-tests.txt):"
    cat "$OUT/$NAME-tests-summary.txt"
    if grep -q '^UNEXPECTED PASS: [1-9]' "$OUT/$NAME-tests-summary.txt"; then
        log "Warning: known failures passed: remove them from Natron-Tests' known_failures/"
    fi
    [ "$status" = 0 ] || log "Error: runTests.sh exited with status $status (a test failed that isn't a known failure, or the run failed)"
elif [ -f "$OUT/$NAME-tests.txt" ]; then
    log "Error: no summary.txt: runTests.sh did not finish (see $OUT/$NAME-tests.log)"
    status=1
else
    log "Error: no result.txt (see $OUT/$NAME-tests.log)"
    status=1
fi
[ "$status" = 0 ] || exit 1
