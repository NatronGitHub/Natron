#!/bin/bash
# Run a Natron binary (Natron, NatronRenderer, Tests, natron-python...) under lldb in batch mode.
# On a crash, lldb catches it before macOS ReportCrash does, so no "quit unexpectedly" dialog is
# shown. It prints the backtrace of all threads, kills the process and exits with status 139.
# Otherwise the exit status is the program's own.
# target.inherit-tcc: the program inherits the terminal's privacy (TCC) permissions instead of
# becoming "responsible" itself, so macOS does not ask "Natron would like to access files in your
# Documents folder" for every new build.
# stdin is /dev/null, so `NatronRenderer -t script.py` doesn't wait for interactive input, or the
# file given by NATRON_LLDB_STDIN (e.g. the commands for the -t prompt).
# Usage: natron-lldb.sh [-o backtrace.txt] /path/to/NatronRenderer args...
set -u
out=""
if [ "${1:-}" = "-o" ]; then out="$2"; shift 2; fi
prog="$1"; shift
# Sanitizer builds: report and exit instead of abort() (abort would also trigger ReportCrash).
export ASAN_OPTIONS="${ASAN_OPTIONS:-abort_on_error=0:detect_leaks=0}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-abort_on_error=0:print_stacktrace=1}"
export TSAN_OPTIONS="${TSAN_OPTIONS:-abort_on_error=0}"
log=$(mktemp -t natron-lldb)
stdin="${NATRON_LLDB_STDIN:-/dev/null}"

# lldb's inferior runs in its own process group, under debugserver: killing lldb, or its process
# group, leaves it running. On TERM/INT/HUP (e.g. a test timeout), kill the whole tree.
descendants() {
  local c
  for c in $(pgrep -P "$1"); do
    descendants "$c"
    echo "$c"
  done
}
child=""
cleanup() {
  trap - TERM INT HUP
  if [ -n "$child" ]; then
    pids="$(sed -n 's/^Process \([0-9]*\) launched.*/\1/p' "$log" 2>/dev/null) $(descendants "$child")"
    # shellcheck disable=SC2086
    kill -KILL $pids 2>/dev/null
    kill -KILL "$child" 2>/dev/null
    wait "$child" 2>/dev/null
  fi
  cat "$log" 2>/dev/null
  rm -f "$log"
  echo "natron-lldb.sh: interrupted, killed $prog" >&2
  exit 143
}
trap cleanup TERM INT HUP

/usr/bin/lldb --batch --no-lldbinit \
  -o "settings set target.inherit-tcc true" \
  -o "settings set target.input-path '$stdin'" \
  -o "process handle SIGPIPE -n true -p true -s false" \
  -o run \
  -k "thread backtrace all" \
  -k "process kill" \
  -k "quit 139" \
  -- "$prog" "$@" > "$log" 2>&1 < /dev/null &
child=$!
wait "$child"
if grep -q "attach failed (Not allowed to attach" "$log"; then
  # Hardened-runtime binaries without get-task-allow (e.g. a release Natron.app) can't be
  # debugged. Run them directly instead, with the same stdin. There is no backtrace on a crash,
  # and a GUI app may show the macOS crash dialog, so prefer a local (debuggable) build.
  rm -f "$log"
  echo "natron-lldb.sh: lldb may not attach to $prog (hardened runtime); running it directly" >&2
  "$prog" "$@" < "$stdin" &
  child=$!
  wait "$child"
  exit $?
fi
cat "$log"
# lldb --batch doesn't propagate the inferior's status: derive it from the log.
if grep -q "stop reason = " "$log"; then
  rc=139
  [ -n "$out" ] && cp "$log" "$out"
else
  rc=$(sed -n 's/^Process [0-9]* exited with status = \([0-9]*\).*/\1/p' "$log" | tail -1)
  rc=${rc:-1}
fi
rm -f "$log"
exit $rc
