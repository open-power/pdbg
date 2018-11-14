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
#    other functions.
#
# test_setup <shell-cmd>
#
#    Register a setup function to be executed before running the tests.
#
# test_cleanup <shell-cmd>
#
#    Register a cleanup function to be executed after running the tests.
#
# test_result <rc> <--|output>
#
#    Define the exit code and the output for the test.  If there is no output
#    from the commands, then '--' can be specified to denote empty output.
#    Multi-line output can be added using a here document.  Only the output
#    on stdout is captured.
#
# test_result_stderr <output>
#
#    Define the output to stderr for the test.  If there is no output from
#    the commands, or if matching of output on stderr is not required, then
#    this command can be ommitted.  Multi-line output can be specified
#    using a here document.
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
# test_wrapper
#
#    Register a test wrapper function.  If test_wrapper is called without
#    an argument, then the test wrapper function is set to default.
#    The test wrapper function will be passed all the arguments to test_run
#    command.
#
#
# Matching output:
#
#     To match varying output, define result_filter() to filter the output.
#     This allows matching date or time stamps.
#
#
# Example:
#
#  test_setup "touch somefile"
#  test_cleanup "rm -f somefile"
#
#  test_group "my group of tests"
#
#  test_result 0 output1
#  test_run command1 arguments1
#
#  test_result 1 --
#  test_result_stderr stderr2
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
test_stderr=0

count_total=0
count_skipped=0
count_failed=0

trap 'test_end' 0

test_setup_hooks=""

test_setup ()
{
	test_setup_hooks="${test_setup_hooks}${test_setup_hooks:+ && }$*"
}

test_cleanup_hooks=""

test_cleanup ()
{
	test_cleanup_hooks="${test_cleanup_hooks}${test_cleanup_hooks:+ ; }$*"
}

test_end ()
{
	trap 0

	eval $test_cleanup_hooks

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
	trap 0
	echo "$@"
	exit 99
}

test_error_skip ()
{
	trap 0
	echo "$@"
	exit 77
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

test_wrapper_default ()
{
	"$@"
}

test_wrapper_func=test_wrapper_default

test_wrapper ()
{
	if [ $# -eq 0 ] ; then
		test_wrapper_func=test_wrapper_default
	else
		test_wrapper_func="$1"
	fi
}

result_filter_default ()
{
	cat
}

result_filter ()
{
	result_filter_default
}

#---------------------------------------------------------------------
# Public functions
#---------------------------------------------------------------------

test_group ()
{
	test_name=${1:-$test_name}
	test_started=1

	echo "-- $test_name"

	eval $test_setup_hooks
	rc=$?
	if [ $rc -ne 0 ] ; then
		if [ $rc -eq 77 ] ; then
			test_error_skip "setup failed, skipping"
		else
			test_error "ERROR: setup failed"
		fi
	fi
}

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

test_result_stderr ()
{
	if [ $test_started -eq 0 ] ; then
		test_error "ERROR: missing call to test_group"
	fi
	if [ $test_defined -eq 0 ] ; then
		test_error "ERROR: missing call to test_result"
	fi

	if [ $# -eq 1 ] ; then
		required_output_stderr="$1"
	else
		if ! tty -s ; then
			required_output_stderr=$(cat)
		else
			required_output_stderr=""
		fi
	fi

	test_stderr=1
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

	stderr_file=$(mktemp)

	output_raw=$($test_wrapper_func "$@" 2>"$stderr_file")
	rc=$?

	if [ $rc -ne $required_rc ] ; then
		test_log "expected rc: $required_rc"
		test_log "output rc: $rc"
	fi

	output=$(echo "$output_raw" | result_filter)
	if [ "$output" != "$required_output" ] ; then
		test_log "expected:"
		test_log "$required_output"
		test_log "output:"
		test_log "$output"
		if [ "$output_raw" != "$output" ] ; then
			test_log "output raw:"
			test_log "$output_raw"
		fi
		output_mismatch=1
	fi

	if [ $test_stderr -eq 1 ] ; then
		output_stderr_raw=$(cat "$stderr_file")
		output_stderr=$(cat "$stderr_file" | result_filter)
		if [ "$output_stderr" != "$required_output_stderr" ] ; then
			test_log "expected stderr:"
			test_log "$required_output_stderr"
			test_log "output stderr:"
			test_log "$output_stderr"
			if [ "$output_stderr_raw" != "$output_stderr" ] ; then
				test_log "output stderr raw:"
				test_log "$output_stderr_raw"
			fi
			output_mismatch=1
		fi
	fi

	rm -f "$stderr_file"

	test_output $rc $required_rc $output_mismatch
	test_skipped=0
	test_defined=0
	test_stderr=0
}
