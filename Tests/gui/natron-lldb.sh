#!/bin/bash
# Run a Natron binary (Natron, NatronRenderer, Tests, natron-python...) under lldb in batch mode.
# On a crash, lldb catches it before macOS ReportCrash does, so no "quit unexpectedly" dialog is
# shown. It prints the backtrace of all threads, kills the process and exits with status 139.
# Otherwise the exit status is the program's own.
# target.inherit-tcc: the program inherits the terminal's privacy (TCC) permissions instead of
# becoming "responsible" itself, so macOS does not ask "Natron would like to access files in your
# Documents folder" for every new build.
# stdin is /dev/null, so `NatronRenderer -t script.py` doesn't wait for interactive input.
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
/usr/bin/lldb --batch --no-lldbinit \
  -o "settings set target.inherit-tcc true" \
  -o "settings set target.input-path /dev/null" \
  -o "process handle SIGPIPE -n true -p true -s false" \
  -o run \
  -k "thread backtrace all" \
  -k "process kill" \
  -k "quit 139" \
  -- "$prog" "$@" > "$log" 2>&1
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
