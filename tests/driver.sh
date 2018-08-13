#!/bin/sh

#
# Simple test driver for shell based testsuite
#
# Each testsuite is a shell script, which sources this driver file.
# Following functions can be used to define testsuite and tests.
#
# test_group <testsuite-name>
#
#    Define a test suite.  This function should be called before calling any
#    other functions
#
# test_result <rc> <--|output>
#
#    Define the exit code and the output for the test.  If there is no output
#    from the commands, then '--' can be specified to denote empty output.
#    Multi-line output can be added using a here document.
#
# test_run <command> [<arguments>]
#
#    Define the command to execute for the test.
#
# test_skip
#
#    This can be called before test_run, to skip the test.  This is useful to
#    write tests which are dependent on the environment (e.g. architecture).
#
#
# Example:
#
#  test_group "my group of tests"
#
#  test_result 0 output1
#  test_run command1 arguments1
#
#  test_result 1 --
#  test_run command2 arguments2
#
#  test_result 0 output3
#  if [ $condition ] ; then
#       test_skip
#  fi
#  test_run command3 arguments3
#

set -u

TESTDIR=$(dirname "$0")
if [ "$TESTDIR" = "." ] ; then
	TESTDIR=$(cd "$TESTDIR"; pwd)
fi
SRCDIR=$(dirname "$TESTDIR")
PATH="$SRCDIR":$PATH

test_name=${TEST_NAME:-$0}
test_logfile=${TEST_LOG:-}
test_trsfile=${TEST_TRS:-}
test_color=${TEST_COLOR:-yes}

red= grn= lgn= blu= mgn= std=
if [ $test_color = yes ] ; then
	red='[0;31m' # Red.
	grn='[0;32m' # Green.
	lgn='[1;32m' # Light green.
	blu='[1;34m' # Blue.
	mgn='[0;35m' # Magenta.
	std='[m'     # No color.
fi

test_started=0
test_skipped=0
test_defined=0

count_total=0
count_skipped=0
count_failed=0

trap 'test_end' 0

test_group ()
{
	test_name=${1:-$test_name}
	test_started=1

	echo "-- $test_name"
}

test_end ()
{
	trap 0
	if [ $count_total -eq 0 ] ; then
		status=99
	elif [ $count_failed -gt 0 ] ; then
		status=1
	elif [ $count_skipped -eq $count_total ] ; then
		status=77
	else
		status=0
	fi

	exit $status
}

test_error ()
{
	echo "$@"
	exit 99
}

test_log ()
{
	if [ -z "$test_logfile" ] ; then
		echo "$@"
	else
		echo "$@" >> "$test_logfile"
	fi
}

test_trs ()
{
	if [ -n "$test_trsfile" ] ; then
		echo "$@" >> "$test_trsfile"
	fi
}

test_output ()
{
	rc=${1:-1}
	required_rc=${2:-0}
	output_mismatch=${3:-0}

	if [ $required_rc -eq 0 ] ; then
		expect_failure=no
	else
		expect_failure=yes
	fi

	case $rc:$expect_failure:$output_mismatch in
	0:*:1)   col=$red res=FAIL ;;
	0:yes:*) col=$red res=XPASS ;;
	0:*:*)   col=$grn res=PASS ;;
	77:*:*)  col=$blu res=SKIP ;;
	*:*:1)   col=$red res=FAIL ;;
	*:yes:*) col=$lgn res=XFAIL ;;
	*:*:*)   col=$red res=FAIL ;;
	esac

	if [ -n "$test_logfile" ] ; then
		test_log "${res} ${test_cmd} (exit status: ${rc})"
	fi
	test_trs ":test-result: ${res}"

	if [ -n "$test_trsfile" ] ; then
		indent="  "
	else
		indent=""
	fi

	echo "${indent}${col}${res}${std}: ${test_cmd}"

	count_total=$(( count_total + 1 ))
	if [ $res = "SKIP" ] ; then
		count_skipped=$(( count_skipped + 1 ))
	elif [ $res = "XPASS" -o $res = "FAIL" ] ; then
		count_failed=$(( count_failed + 1 ))
	fi
}

#---------------------------------------------------------------------
# Public functions
#---------------------------------------------------------------------

test_result ()
{
	if [ $test_started -eq 0 ] ; then
		test_error "ERROR: missing call to test_group"
	fi

	required_rc="${1:-0}"
	if [ $# -eq 2 ] ; then
		if [ "$2" = "--" ] ; then
			required_output=""
		else
			required_output="$2"
		fi
	else
		if ! tty -s ; then
			required_output=$(cat)
		else
			required_output=""
		fi
	fi

	test_defined=1
}

test_skip ()
{
	if [ $test_started -eq 0 ] ; then
		test_error "ERROR: missing call to test_group"
	fi

	test_skipped=1
}

test_run ()
{
	output_mismatch=0

	if [ $test_started -eq 0 ] ; then
		test_error "ERROR: missing call to test_group"
	fi

	if [ $test_defined -eq 0 ] ; then
		test_error "ERROR: missing call to test_result"
	fi

	test_cmd="$@"

	if [ $test_skipped -eq 1 ] ; then
		test_output 77
		test_skipped=0
		test_defined=0
		return
	fi

	output=$("$@" 2>&1)
	rc=$?

	if [ $rc -ne $required_rc ] ; then
		test_log "expected rc: $required_rc"
		test_log "output rc: $rc"
	fi

	if [ "$output" != "$required_output" ] ; then
		test_log "expected:"
		test_log "$required_output"
		test_log "output:"
		test_log "$output"
		output_mismatch=1
	fi

	test_output $rc $required_rc $output_mismatch
	test_skipped=0
	test_defined=0
}
