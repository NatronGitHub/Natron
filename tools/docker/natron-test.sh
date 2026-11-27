#!/bin/bash
# Run the Natron-Tests regression suite on a Linux portable archive, in a clean, throw-away
# Rocky Linux 9 container ($TEST_IMAGE): no SDK, no compiler, no development packages. Only the
# system libraries that the Linux binaries never bundle (INSTALL_LINUX.md, "Runtime requirements
# of the Linux binaries") and the tools that runTests.sh needs are installed. This checks that
# the archive is self-contained and runs on the oldest supported distribution. It is the Linux
# counterpart of tools/tart/natron-test.sh.
# With -H, the suite runs directly on this machine instead, as the current user, with a new
# empty HOME and no container: linux_packages.yml uses it on the latest Ubuntu runners, to check
# the archive on the newest systems (#1964, #1965). The caller installs the runtime packages.
#
#   natron-test.sh [options] Natron-...-Linux-<arch>-no-installer.tar.xz
#     -o DIR      where to write the results (default: the archive's directory)
#     -n DIR      Natron-Tests checkout (default: $NATRON_TESTS_SRC, cloned if missing)
#     -H          run on this machine, without a container
#
# DOCKER (default: docker) can be set to podman. TEST=TestFoo or TESTCUSTOM=TestBar run a single
# test (see runTests.sh); the results are then named <name>-partial-<test>-*.
# runTests.sh reports the tests listed in Natron-Tests' known_failures/common.txt and linux.txt as
# "KNOWN FAIL", and those of them that passed as "UNEXPECTED PASS" (also arm64.txt or x86_64.txt).
# Exit status: 0 if every test passed or is a known failure, 1 otherwise: a test failed that isn't
# a known failure, a known failure passed (UNEXPECTED PASS), or the suite did not run.
# Results, named after the archive (<name> is the archive name without -no-installer.tar.xz):
#   <name>-tests.txt                  runTests.sh's result.txt
#   <name>-unit_tests_failures.zip    the failed tests' outputs and references
#   <name>-tests.log                  runTests.sh's output
#   <name>-tests-summary.txt          the counts of PASS, FAIL, KNOWN FAIL, UNEXPECTED PASS
set -euo pipefail

DOCKER=${DOCKER:-docker}
TEST_IMAGE=${TEST_IMAGE:-rockylinux/rockylinux:9}
NATRON_TESTS_SRC=${NATRON_TESTS_SRC:-$PWD/Natron-Tests}
NATRON_TESTS_URL=${NATRON_TESTS_URL:-https://github.com/NatronGitHub/Natron-Tests.git}
TEST_TIMEOUT=${TEST_TIMEOUT:-14400}   # seconds, for the whole suite
OUT=
HOST=false
while getopts "o:n:Hh" opt; do
    case $opt in
        o) OUT=$OPTARG ;;
        n) NATRON_TESTS_SRC=$OPTARG ;;
        H) HOST=true ;;
        *) sed -n 2,27p "$0"; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
[ $# = 1 ] && [ -f "$1" ] || { sed -n 2,27p "$0"; exit 1; }
ARCHIVE=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
NAME=$(basename "$ARCHIVE" .tar.xz)
NAME=${NAME%-no-installer}
OUT=${OUT:-$(dirname "$ARCHIVE")}
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)
if [ -n "${TEST:-}${TESTCUSTOM:-}" ]; then
    NAME="$NAME-partial-$(echo "${TEST:-}${TESTCUSTOM:-}" | tr -c 'A-Za-z0-9._\n-' _)"
fi
rm -f "$OUT/$NAME-tests.log" "$OUT/$NAME-tests.txt" "$OUT/$NAME-tests-summary.txt" "$OUT/$NAME-unit_tests_failures.zip"

if [ ! -d "$NATRON_TESTS_SRC/.git" ]; then
    echo "*** cloning $NATRON_TESTS_URL into $NATRON_TESTS_SRC"
    git clone --depth 1 "$NATRON_TESTS_URL" "$NATRON_TESTS_SRC"
fi
NATRON_TESTS_SRC=$(cd "$NATRON_TESTS_SRC" && pwd)
echo "*** Natron-Tests: $(git -C "$NATRON_TESTS_SRC" log -1 --format='%h %cs %s')"
[ -f "$NATRON_TESTS_SRC/known_failures/linux.txt" ] ||
    { echo "Error: $NATRON_TESTS_SRC has no known_failures/linux.txt (NatronGitHub/Natron-Tests#3)"; exit 1; }

# runTests.sh downloads two example projects from SourceForge, which is sometimes slow or flaky:
# fetch them once, next to the tests (runTests.sh uses them when they are valid zips)
for z in Natron_2.3.12_Spaceship.zip Natron_2.3.12_BayMax.zip; do
    if ! unzip -tqq "$NATRON_TESTS_SRC/$z" > /dev/null 2>&1; then
        echo "*** downloading $z"
        { curl -fsSL --retry 3 --max-time 1800 -o "$NATRON_TESTS_SRC/$z" "https://downloads.sourceforge.net/project/natron/Examples/$z" &&
            unzip -tqq "$NATRON_TESTS_SRC/$z" > /dev/null 2>&1; } ||
            { rm -f "$NATRON_TESTS_SRC/$z"; echo "Warning: could not download $z (runTests.sh will try again)"; }
    fi
done

# Container only, as root: installs the runtime packages and creates the user that runs RUN.
# shellcheck disable=SC2016
SETUP='set -euo pipefail
# the system libraries that are never bundled (see SYSTEM_LIBS_RE in build-Linux-installer.sh),
# fontconfig and a font (every desktop has them), and the tools that runTests.sh uses
dnf -y -q install libgomp libglvnd-glx libglvnd-egl libglvnd-opengl \
    libX11 libX11-xcb libXext libXrender libXi libXfixes libXrandr libXcursor libXinerama \
    libXcomposite libXdamage libXtst libSM libICE libxcb xcb-util-wm xcb-util-image \
    xcb-util-keysyms xcb-util-renderutil libxkbcommon libxkbcommon-x11 alsa-lib \
    fontconfig dejavu-sans-fonts \
    findutils diffutils procps-ng xz unzip zip shadow-utils util-linux > /dev/null
dnf clean all > /dev/null
useradd -m natron
'
# Extracts the archive into $H, runs the suite as $U (through su if RUNAS is set, i.e. as root in
# the container) and writes the results to $OUTD. IN has archive.tar.xz and Natron-Tests.
# shellcheck disable=SC2016
RUN='set -euo pipefail
cd "$H"
tar -xJf "$IN/archive.tar.xz"
mv ./*-no-installer natron
mkdir Natron-Tests
cp -a "$IN/Natron-Tests/." Natron-Tests/
rm -rf Natron-Tests/.git Natron-Tests/result.txt Natron-Tests/summary.txt Natron-Tests/failed
[ -z "$RUNAS" ] || chown -R "$RUNAS": natron Natron-Tests
P=$H/natron
# a missing library would make every test fail: list them first
missing=$(find "$P" -type f \( -name "*.so*" -o -name "*.ofx" -o -perm -u+x \) | while IFS= read -r f; do
    ldd "$f" 2> /dev/null | grep "not found" | sed "s#^#$f: #" || true
done)
status=0
[ -z "$missing" ] || { echo "Error: missing system libraries:"; echo "$missing"; status=1; }
# cmd is parsed again by su or bash: quote the paths (TMPDIR may contain spaces in host mode)
qH=$(printf %q "$H"); qU=$(printf %q "$U"); qP=$(printf %q "$P")
cmd="cd Natron-Tests && env -i HOME=$qH USER=$qU LOGNAME=$qU SHELL=/bin/bash \
    LANG=C.UTF-8 PATH=/usr/bin:/bin:/usr/sbin:/sbin \
    ${TEST:+TEST=\"$TEST\"} ${TESTCUSTOM:+TESTCUSTOM=\"$TESTCUSTOM\"} \
    NATRON_DISK_CACHE_PATH=$qH/NatronTmpCacheDir \
    OCIO=$qP/Resources/OpenColorIO-Configs/blender/config.ocio \
    FFMPEG=$qP/bin/ffmpeg COMPARE=$qP/bin/idiff \
    timeout -s KILL $TEST_TIMEOUT /bin/bash runTests.sh $qP/NatronRenderer"
if [ -n "$RUNAS" ]; then
    su "$RUNAS" -c "$cmd"
else
    bash -c "$cmd"
fi > "$OUTD/runTests.log" 2>&1 || status=$?
[ -z "$missing" ] || { echo "Error: missing system libraries:"; echo "$missing"; } >> "$OUTD/runTests.log"
echo "runTests.sh exited, status $status (also 1 if a system library is missing)"
cd Natron-Tests
# runTests.sh only copies the images of the failed render tests to failed/: also copy those of
# the failed custom tests (TestCMD, TestWriteFFmpeg/<format>, ...), as in tools/tart/natron-test.sh
if [ -f result.txt ]; then
    mkdir -p failed
    { grep -E " : (KNOWN )?FAIL\$" result.txt || true; } | sed -E "s/ : (KNOWN )?FAIL\$//" | while IFS= read -r t; do
        case $t in */*)
            d=${t%%/*}/formats/${t#*/}
            if [ -d "$d" ]; then
                for f in "$d"/reference*.jpg "$d"/output*.jpg "$d"/comp*.jpg "$d"/reference*.png "$d"/output*.png "$d"/comp*.png; do
                    [ ! -f "$f" ] || cp "$f" "failed/$(echo "$t" | tr / -)-$(basename "$f")"
                done
            else
                d=${t%/*}; n=${t##*/}
                for f in "$d/reference$n.jpg" "$d/output$n.jpg" "$d/comp$n.jpg" "$d/reference$n.png" "$d/output$n.png" "$d/comp$n.png"; do
                    [ ! -f "$f" ] || cp "$f" "failed/$(echo "$d" | tr / -)-$(basename "$f")"
                done
            fi ;;
        esac
    done
    cp result.txt "$OUTD/result.txt"
fi
[ ! -f summary.txt ] || cp summary.txt "$OUTD/summary.txt"
if [ -n "$(ls -A failed 2> /dev/null)" ]; then
    mv failed "$OUTD/$NAME-unit_tests_failures"
    (cd "$OUTD" && zip -r -q "$NAME-unit_tests_failures.zip" "$NAME-unit_tests_failures" && rm -rf "$NAME-unit_tests_failures")
fi
exit "$status"'

WORK=$(mktemp -d "${TMPDIR:-/tmp}/natron-test.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
status=0
if [ "$HOST" = true ]; then
    # RUN writes only in $WORK (HOME too); the archive and Natron-Tests are only read
    mkdir "$WORK/in" "$WORK/home"
    ln -s "$ARCHIVE" "$WORK/in/archive.tar.xz"
    ln -s "$NATRON_TESTS_SRC" "$WORK/in/Natron-Tests"
    # env -i: neither the missing-library check nor the suite sees this shell's environment
    # (e.g. a LD_LIBRARY_PATH that would hide a library missing from the archive)
    env -i PATH=/usr/bin:/bin:/usr/sbin:/sbin LANG=C.UTF-8 NAME="$NAME" TEST_TIMEOUT="$TEST_TIMEOUT" TEST="${TEST:-}" TESTCUSTOM="${TESTCUSTOM:-}" \
        IN="$WORK/in" OUTD="$WORK" H="$WORK/home" U="$(id -un)" RUNAS= \
        bash -c "$RUN" || status=$?
    echo "*** the test run exited with status $status"
else
    # no port is published and nothing is written outside $WORK; the archive and Natron-Tests are read-only
    # shellcheck disable=SC2086 # DOCKER may have options, e.g. "podman --connection NAME"
    $DOCKER run --rm --pull always \
        -v "$ARCHIVE:/in/archive.tar.xz:ro" -v "$NATRON_TESTS_SRC:/in/Natron-Tests:ro" -v "$WORK:/out" \
        -e NAME="$NAME" -e TEST_TIMEOUT="$TEST_TIMEOUT" -e TEST="${TEST:-}" -e TESTCUSTOM="${TESTCUSTOM:-}" \
        -e IN=/in -e OUTD=/out -e H=/home/natron -e U=natron -e RUNAS=natron \
        "$TEST_IMAGE" bash -c "$SETUP$RUN" || status=$?
    echo "*** the test container exited with status $status"
fi
[ ! -f "$WORK/runTests.log" ] || mv "$WORK/runTests.log" "$OUT/$NAME-tests.log"
[ ! -f "$WORK/result.txt" ] || mv "$WORK/result.txt" "$OUT/$NAME-tests.txt"
[ ! -f "$WORK/summary.txt" ] || mv "$WORK/summary.txt" "$OUT/$NAME-tests-summary.txt"
[ ! -f "$WORK/$NAME-unit_tests_failures.zip" ] || mv "$WORK/$NAME-unit_tests_failures.zip" "$OUT/"

if [ -f "$OUT/$NAME-tests.txt" ] && [ -f "$OUT/$NAME-tests-summary.txt" ]; then
    echo "*** Natron-Tests ($OUT/$NAME-tests.txt):"
    cat "$OUT/$NAME-tests-summary.txt"
    [ "$status" = 0 ] || echo "Error: status $status (a test failed that isn't a known failure, a known failure passed, a system library is missing, or the run failed)"
    # also with a Natron-Tests whose runTests.sh doesn't fail on unexpected passes yet
    if grep -q '^UNEXPECTED PASS: [1-9]' "$OUT/$NAME-tests-summary.txt"; then
        echo "Error: known failures passed (UNEXPECTED PASS): remove them from Natron-Tests' known_failures/"
        status=1
    fi
elif [ -f "$OUT/$NAME-tests.txt" ]; then
    echo "Error: no summary.txt: runTests.sh did not finish (see $OUT/$NAME-tests.log)"
    status=1
else
    echo "Error: no result.txt (see $OUT/$NAME-tests.log)"
    status=1
fi
exit "$status"
