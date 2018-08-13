#!/bin/sh

TEST_COLOR=no
TEST_LOG=/dev/null
TEST_TRS=""

. $(dirname "$0")/driver.sh

test_group "test driver tests"

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
