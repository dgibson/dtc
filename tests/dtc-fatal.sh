#! /bin/sh

# Usage: dtc-fatal.sh [-m <message>] <dtc args>
# Check that dtc fails, and with -m that its output contains <message>

SRCDIR=`dirname "$0"`
. "$SRCDIR/testutils.sh"

if [ "$1" = "-m" ]; then
    msg="$2"
    shift 2
fi

LOG=tmp.log.$$
rm -f $LOG
trap "rm -f $LOG" 0

verbose_run_log "$LOG" $VALGRIND "$DTC" -o/dev/null "$@"
ret="$?"

if [ "$ret" -gt 127 ]; then
    FAIL "dtc killed by signal (ret=$ret)"
elif [ "$ret" != "1" ]; then
    FAIL "dtc returned incorrect status $ret instead of 1"
fi

if [ -n "$msg" ] && ! grep -qF -- "$msg" "$LOG"; then
    FAIL "Expected message '$msg'"
fi

PASS
