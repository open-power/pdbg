#!/bin/sh

TEST_COLOR=no
TEST_LOG=/dev/null
TEST_TRS=""

. $(dirname "$0")/driver.sh

test_setup "echo test start"
test_cleanup "echo test end"

test_setup "echo real start"
test_cleanup "echo real end"

test_group "test driver tests"

echo

echo "test should PASS"
test_result 0 foo
test_run echo foo

echo "test should FAIL"
test_result 0 foo
test_run echo bar

echo "test should XPASS"
test_result 1 foo
test_run echo foo

echo "test should FAIL"
test_result 1 foo
test_run echo bar

echo "test should XFAIL"
test_result 1 --
test_run false

echo "test should FAIL"
test_result 1 --
test_run echo foo && false

echo "test should SKIP"
test_result 0 --
test_skip
test_run echo foo

echo

echo_stderr ()
{
	echo "$*" >&2
}

test_wrapper echo_stderr

echo "match stderr"
test_result 0 --
test_result_stderr foo
test_run foo

test_wrapper

echo

result_filter ()
{
	sed -e 's#[0-9][0-9][0-9]#NUM3#g' \
	    -e 's#[0-9][0-9]#NUM2#g'
}

test_result 0 NUM2
test_run echo 42

test_result 0 NUM3
test_run echo 666

echo

prefix_output ()
{
	echo "output: $*"
}

test_wrapper prefix_output

test_result 0 "output: foobar"
test_run foobar

echo
