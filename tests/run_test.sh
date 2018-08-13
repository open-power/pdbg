#!/bin/sh

# Derived from automake basic testsuite driver

set -u

usage_error ()
{
	echo "$0: $*" >&2
	print_usage >&2
	exit 2
}

print_usage ()
{
	cat <<EOF
Usage: $0 --test-name NAME --log-file PATH --trs-file PATH
	    [--expect-failure={yes|no}] [--color-tests={yes|no}]
	    [--enable-hard-errors={yes|no}] [--]
	    TEST-SCRIPT [TEST-SCRIPT-ARGUMENTS]
The '--test-name', '--log-file' and '--trs-file' options are mandatory.
EOF
}

TEST_NAME=
TEST_LOG=
TEST_TRS=
TEST_COLOR=no
expect_failure=no
enable_hard_errors=yes

while [ $# -gt 0 ] ; do
	case "$1" in
	--help) print_usage; exit $?;;
	--test-name) TEST_NAME=$2; shift;;
	--log-file) TEST_LOG="$2"; shift;;
	--trs-file) TEST_TRS="$2"; shift;;
	--color-tests) TEST_COLOR=$2; shift;;
	--expect-failure) expect_failure=$2; shift;;
	--enable-hard-errors) enable_hard_errors=$2; shift;;
	--) shift; break;;
	-*) usage_error "invalid option: '$1'";;
	*) break;;
	esac
	shift
done

missing_opts=""
[ -z "$TEST_NAME" ] && missing_opts="$missing_opts --test-name"
[ -z "$TEST_LOG" ] && missing_opts="$missing_opts --log-file"
[ -z "$TEST_TRS" ] && missing_opts="$missing_opts --trs-file"
if [ -n "$missing_opts" ] ; then
	usage_error "these mandatory options are missing:$missing_opts"
fi

if [ $# -eq 0 ] ; then
	usage_error "missing argument"
fi

do_exit='rm -f $log_file $trs_file; (exit $st) ; exit $st'
trap "st=129; $do_exit" 1
trap "st=130; $do_exit" 2
trap "st=141; $do_exit" 13
trap "st=143; $do_exit" 15

export TEST_NAME TEST_LOG TEST_TRS TEST_COLOR

# Test script is run here.
"$@"
estatus=$?

if [ $enable_hard_errors = no -a $estatus -eq 99 ] ; then
	tweaked_estatus=1
else
	tweaked_estatus=$estatus
fi

case $tweaked_estatus:$expect_failure in
0:yes) res=XPASS recheck=yes gcopy=yes ;;
0:*)   res=PASS  recheck=no  gcopy=no  ;;
77:*)  res=SKIP  recheck=no  gcopy=yes ;;
99:*)  res=ERROR recheck=yes gcopy=yes ;;
*:yes) res=XFAIL recheck=no  gcopy=yes ;;
*:*)   res=FAIL  recheck=yes gcopy=yes ;;
esac

echo "$res $TEST_NAME (exit_status: $estatus)" >> "$TEST_LOG"

echo ":global-test-result: $res" >> "$TEST_TRS"
echo ":recheck: $recheck" >> "$TEST_TRS"
echo ":copy-in-global-log: $gcopy" >> "$TEST_TRS"
