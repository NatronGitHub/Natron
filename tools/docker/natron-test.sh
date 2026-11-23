#!/bin/bash
# Run the Natron-Tests regression suite on a Linux portable archive, in a clean, throw-away
# Rocky Linux 9 container ($TEST_IMAGE): no SDK, no compiler, no development packages. Only the
# system libraries that the Linux binaries never bundle (INSTALL_LINUX.md, "Runtime requirements
# of the Linux binaries") and the tools that runTests.sh needs are installed. This checks that
# the archive is self-contained and runs on the oldest supported distribution. It is the Linux
# counterpart of tools/tart/natron-test.sh.
#
#   natron-test.sh [options] Natron-...-Linux-<arch>-no-installer.tar.xz
#     -o DIR      where to write the results (default: the archive's directory)
#     -n DIR      Natron-Tests checkout (default: $NATRON_TESTS_SRC, cloned if missing)
#
# DOCKER (default: docker) can be set to podman. TEST=TestFoo or TESTCUSTOM=TestBar run a single
# test (see runTests.sh); the results are then named <name>-partial-<test>-*.
# runTests.sh reports the tests listed in Natron-Tests' known_failures/common.txt and linux.txt as
# "KNOWN FAIL", and those of them that passed as "UNEXPECTED PASS".
# Exit status: 0 if every test passed or is a known failure, 1 otherwise (or if the suite did not
# run). Results, named after the archive (<name> is the archive name without -no-installer.tar.xz),
# as in the Natron release assets:
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
while getopts "o:n:h" opt; do
    case $opt in
        o) OUT=$OPTARG ;;
        n) NATRON_TESTS_SRC=$OPTARG ;;
        *) sed -n 2,23p "$0"; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
[ $# = 1 ] && [ -f "$1" ] || { sed -n 2,23p "$0"; exit 1; }
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

# Runs as root in the container: installs the runtime packages, then runs the suite as a user.
# shellcheck disable=SC2016
INNER='set -euo pipefail
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
cd /home/natron
tar -xJf /in/archive.tar.xz
mv ./*-no-installer natron
cp -a /in/Natron-Tests Natron-Tests
rm -rf Natron-Tests/.git Natron-Tests/result.txt Natron-Tests/summary.txt Natron-Tests/failed
chown -R natron: natron Natron-Tests
P=/home/natron/natron
# a missing library would make every test fail: list them first
missing=$(find "$P" -type f \( -name "*.so*" -o -name "*.ofx" -o -perm -u+x \) | while IFS= read -r f; do
    ldd "$f" 2> /dev/null | grep "not found" | sed "s#^#$f: #" || true
done)
status=0
[ -z "$missing" ] || { echo "Error: missing system libraries:"; echo "$missing"; status=1; }
su natron -c "cd Natron-Tests && env -i HOME=/home/natron USER=natron LOGNAME=natron SHELL=/bin/bash \
    LANG=C.UTF-8 PATH=/usr/bin:/bin:/usr/sbin:/sbin \
    ${TEST:+TEST=\"$TEST\"} ${TESTCUSTOM:+TESTCUSTOM=\"$TESTCUSTOM\"} \
    NATRON_DISK_CACHE_PATH=/home/natron/NatronTmpCacheDir \
    OCIO=$P/Resources/OpenColorIO-Configs/blender/config.ocio \
    FFMPEG=$P/bin/ffmpeg COMPARE=$P/bin/idiff \
    timeout -s KILL $TEST_TIMEOUT /bin/bash runTests.sh $P/NatronRenderer" > /out/runTests.log 2>&1 || status=$?
[ -z "$missing" ] || { echo "Error: missing system libraries:"; echo "$missing"; } >> /out/runTests.log
echo "runTests.sh exited with status $status"
cd Natron-Tests
# runTests.sh only copies the images of the failed render tests to failed/: also copy those of
# the failed custom tests (TestCMD, TestWriteFFmpeg/<format>, ...), as in tools/tart/natron-test.sh
if [ -f result.txt ]; then
    mkdir -p failed
    { grep -E " : (KNOWN )?FAIL\$" result.txt || true; } | sed -E "s/ : (KNOWN )?FAIL\$//" | while IFS= read -r t; do
        case $t in */*)
            d=${t%%/*}/formats/${t#*/}
            if [ -d "$d" ]; then
                for f in "$d"/reference*.jpg "$d"/output*.jpg "$d"/comp*.jpg; do
                    [ ! -f "$f" ] || cp "$f" "failed/$(echo "$t" | tr / -)-$(basename "$f")"
                done
            else
                d=${t%/*}; n=${t##*/}
                for f in "$d/reference$n.jpg" "$d/output$n.jpg" "$d/comp$n.jpg"; do
                    [ ! -f "$f" ] || cp "$f" "failed/$(echo "$d" | tr / -)-$(basename "$f")"
                done
            fi ;;
        esac
    done
    cp result.txt /out/result.txt
fi
[ ! -f summary.txt ] || cp summary.txt /out/summary.txt
if [ -n "$(ls -A failed 2> /dev/null)" ]; then
    mv failed "/out/$NAME-unit_tests_failures"
    (cd /out && zip -r -q "$NAME-unit_tests_failures.zip" "$NAME-unit_tests_failures" && rm -rf "$NAME-unit_tests_failures")
fi
exit "$status"'

WORK=$(mktemp -d "${TMPDIR:-/tmp}/natron-test.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
status=0
# no port is published and nothing is written outside $WORK; the archive and Natron-Tests are read-only
# shellcheck disable=SC2086 # DOCKER may have options, e.g. "podman --connection NAME"
$DOCKER run --rm --pull always \
    -v "$ARCHIVE:/in/archive.tar.xz:ro" -v "$NATRON_TESTS_SRC:/in/Natron-Tests:ro" -v "$WORK:/out" \
    -e NAME="$NAME" -e TEST_TIMEOUT="$TEST_TIMEOUT" -e TEST="${TEST:-}" -e TESTCUSTOM="${TESTCUSTOM:-}" \
    "$TEST_IMAGE" bash -c "$INNER" || status=$?
echo "*** the test container exited with status $status"
[ ! -f "$WORK/runTests.log" ] || mv "$WORK/runTests.log" "$OUT/$NAME-tests.log"
[ ! -f "$WORK/result.txt" ] || mv "$WORK/result.txt" "$OUT/$NAME-tests.txt"
[ ! -f "$WORK/summary.txt" ] || mv "$WORK/summary.txt" "$OUT/$NAME-tests-summary.txt"
[ ! -f "$WORK/$NAME-unit_tests_failures.zip" ] || mv "$WORK/$NAME-unit_tests_failures.zip" "$OUT/"

if [ -f "$OUT/$NAME-tests.txt" ] && [ -f "$OUT/$NAME-tests-summary.txt" ]; then
    echo "*** Natron-Tests ($OUT/$NAME-tests.txt):"
    cat "$OUT/$NAME-tests-summary.txt"
    if grep -q '^UNEXPECTED PASS: [1-9]' "$OUT/$NAME-tests-summary.txt"; then
        echo "Warning: known failures passed: remove them from Natron-Tests' known_failures/"
    fi
    [ "$status" = 0 ] || echo "Error: runTests.sh exited with status $status (a test failed that isn't a known failure, or the run failed)"
elif [ -f "$OUT/$NAME-tests.txt" ]; then
    echo "Error: no summary.txt: runTests.sh did not finish (see $OUT/$NAME-tests.log)"
    status=1
else
    echo "Error: no result.txt (see $OUT/$NAME-tests.log)"
    status=1
fi
exit "$status"
