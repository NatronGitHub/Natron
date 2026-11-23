#!/bin/bash
# Run the Natron-Tests regression suite on a Natron DMG, in a clean, throw-away clone of the stock
# macOS base image ($BASE_IMAGE, macOS 14 Sonoma, the oldest macOS that Natron 2.6 supports): no
# Xcode, no Command Line Tools, no MacPorts, no Homebrew, no Natron sources. Only the DMG's
# Natron.app and the Natron-Tests data are copied in. This checks that the DMG is self-contained.
#
#   natron-test.sh [options] NATRON.dmg
#     -o DIR      where to write the results (default: the DMG's directory)
#     -n DIR      Natron-Tests checkout on the host (default: $NATRON_TESTS_SRC, cloned if missing)
#     -k          keep the test VM afterwards (default: delete it)
#     -i IMAGE    base image to clone (default: $BASE_IMAGE, see tart-common.sh)
#
# TEST=TestFoo or TESTCUSTOM=TestBar in the environment run a single test (see runTests.sh); the
# results are then named <dmg name>-partial-<test>-*, not as release assets.
# runTests.sh reports the known failures (Natron-Tests' known_failures/common.txt and macos.txt) as
# "KNOWN FAIL" and the known failures that passed as "UNEXPECTED PASS" (the lists must then be
# updated, through a Natron-Tests pull request); natron-test.sh needs a Natron-Tests with these lists.
# Exit status: 0 if every test passed or is a known failure, 1 otherwise (or if the suite did
# not run): a release gate. Unexpected passes are reported, and don't fail the run.
# Results, as in the Natron release assets (tools/jenkins/runUnitTests.sh):
#   <dmg name>-tests.txt                  runTests.sh's result.txt (one "Test : PASS", "FAIL",
#                                         "KNOWN FAIL" or "UNEXPECTED PASS" line per test)
#   <dmg name>-unit_tests_failures.zip    the failed tests' outputs and references, in a
#                                         <dmg name>-unit_tests_failures/ directory
#   <dmg name>-tests.log                  runTests.sh's output
#   <dmg name>-tests-summary.txt          the counts of PASS, FAIL, KNOWN FAIL, UNEXPECTED PASS
set -euo pipefail
. "$(dirname "$0")/tart-common.sh"

OUT=
KEEP=0
NATRON_TESTS_SRC=${NATRON_TESTS_SRC:-$THIRD_PARTY/Natron-Tests}
NATRON_TESTS_URL=${NATRON_TESTS_URL:-https://github.com/NatronGitHub/Natron-Tests.git}
TEST_TIMEOUT=${TEST_TIMEOUT:-14400}   # seconds, for the whole suite
while getopts "o:n:ki:h" opt; do
    case $opt in
        o) OUT=$OPTARG ;;
        n) NATRON_TESTS_SRC=$OPTARG ;;
        k) KEEP=1 ;;
        i) BASE_IMAGE=$OPTARG ;;
        *) sed -n 2,26p "$0"; exit 1 ;;
    esac
done
shift $((OPTIND - 1))
[ $# = 1 ] && [ -f "$1" ] || { sed -n 2,26p "$0"; exit 1; }
DMG=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
NAME=$(basename "$DMG" .dmg)
OUT=${OUT:-$(dirname "$DMG")}
mkdir -p "$OUT"
# a partial run (TEST or TESTCUSTOM) must not produce files with the release asset names
if [ -n "${TEST:-}${TESTCUSTOM:-}" ]; then
    NAME="$NAME-partial-$(echo "${TEST:-}${TESTCUSTOM:-}" | tr -c 'A-Za-z0-9._\n-' _)"
fi
# never leave the results of an earlier run next to the DMG, even if this one is aborted
rm -f "$OUT/$NAME-tests.log" "$OUT/$NAME-tests.txt" "$OUT/$NAME-tests-summary.txt" "$OUT/$NAME-unit_tests_failures.zip"

# the test data, on the host (the guest has no git without the Command Line Tools)
if [ ! -d "$NATRON_TESTS_SRC/.git" ]; then
    log "cloning $NATRON_TESTS_URL into $NATRON_TESTS_SRC"
    git clone --depth 1 "$NATRON_TESTS_URL" "$NATRON_TESTS_SRC"
fi
log "Natron-Tests: $(git -C "$NATRON_TESTS_SRC" log -1 --format='%h %cs %s')"
[ -f "$NATRON_TESTS_SRC/known_failures/common.txt" ] ||
    die "$NATRON_TESTS_SRC has no known_failures/common.txt: use a Natron-Tests with the known-failures lists (NatronGitHub/Natron-Tests#3)"
# runTests.sh downloads two example projects from SourceForge, which is sometimes flaky:
# fetch them on the host, once (runTests.sh uses them when they are valid zips)
for z in Natron_2.3.12_Spaceship.zip Natron_2.3.12_BayMax.zip; do
    if ! unzip -tqq "$NATRON_TESTS_SRC/$z" > /dev/null 2>&1; then
        log "downloading $z"
        curl -fL --retry 3 -o "$NATRON_TESTS_SRC/$z" "https://downloads.sourceforge.net/project/natron/Examples/$z" ||
            { rm -f "$NATRON_TESTS_SRC/$z"; log "Warning: could not download $z (runTests.sh will try again)"; }
    fi
done

tart_lock
trap tart_unlock EXIT   # replaced by cleanup once the clone exists
VM=natron-test-$(date +%Y%m%d-%H%M%S)-$$
cleanup() {
    # the tart guest agent (used by vm_stop) ran from /opt/homebrew: shut down through ssh, if
    # the VM got an address (vm_addr would otherwise search for one for 15 minutes)
    if vm_running "$VM" && [ -s "$(vm_addr_cache "$VM")" ]; then
        vm_ssh "$VM" 'sudo /sbin/shutdown -h now' > /dev/null 2>&1 || true
        for _ in $(seq 30); do vm_running "$VM" || break; sleep 5; done
    fi
    if vm_running "$VM"; then vm_stop "$VM" || true; else rm -f "$(vm_addr_cache "$VM")"; fi
    if [ "$KEEP" = 0 ]; then tart delete "$VM" || log "Warning: could not delete $VM"; else log "kept VM $VM"; fi
    tart_unlock
}
log "cloning $BASE_IMAGE into $VM"
tart clone "$BASE_IMAGE" "$VM"
trap cleanup EXIT
vm_configure "$VM"
vm_start "$VM"

# Make the clone bare: remove the Command Line Tools and hide Homebrew (the tart guest agent
# runs from /opt/homebrew: it keeps running from the moved directory, and the VM is not rebooted).
log "removing the Command Line Tools and Homebrew from $VM"
vm_ssh "$VM" 'set -e
    sudo rm -rf /Library/Developer/CommandLineTools
    sudo find /Applications -maxdepth 1 -name "Xcode*.app" -exec rm -rf {} +
    for p in $(pkgutil --pkgs | grep "^com\.apple\.pkg\.CLTools_"); do sudo pkgutil --forget "$p" > /dev/null 2>&1 || true; done
    [ ! -d /opt/homebrew ] || sudo mv /opt/homebrew /opt/homebrew.disabled
    sudo rm -f /etc/paths.d/homebrew
    [ ! -e /opt/local ] && [ ! -e /Library/Developer/CommandLineTools ] && [ ! -e /opt/homebrew ]
    sw_vers'

log "copying $(basename "$DMG"), run-natron-tests.sh and Natron-Tests into $VM"
vm_rsync "$VM" "$DMG" natron.dmg
vm_rsync "$VM" "$(dirname "$0")/run-natron-tests.sh" run-natron-tests.sh
# not the results of a run in the host checkout: they would pass for this run's
vm_rsync "$VM" "$NATRON_TESTS_SRC/" Natron-Tests/ --exclude .git --exclude /result.txt --exclude /summary.txt --exclude /failed --delete

log "running Natron-Tests in $VM (TEST=${TEST:-all})"
status=0
vm_ssh "$VM" "rm -rf results && env ALL_PROXY=$GUEST_PROXY TEST_TIMEOUT=$TEST_TIMEOUT ${TEST:+TEST=$TEST} ${TESTCUSTOM:+TESTCUSTOM=$TESTCUSTOM} \
    /bin/bash run-natron-tests.sh natron.dmg Natron-Tests results '$NAME'" || status=$?

log "copying the results to $OUT"
vm_rsync_back "$VM" results/ "$OUT/" || { log "Error: no results"; status=1; }
exit "$status"
