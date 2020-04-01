#!/bin/sh

. $(dirname "$0")/driver.sh

FAKE_DTB=fake.dtb
TEST_DTB=fake-attr.dtb

export PDBG_DTB=$TEST_DTB

test_setup cp $FAKE_DTB $TEST_DTB
test_cleanup rm -f $TEST_DTB

test_group "libpdbg array attribute tests"

test_result 0 <<EOF
0x00 
EOF
test_run libpdbg_attr_test / read ATTR1 array 1 1

for size in 2 4 8 ; do
	test_result 88 --
	test_run libpdbg_attr_test / read ATTR1 array $size 1
done

test_result 0 <<EOF
0x0000 
EOF
test_run libpdbg_attr_test / read ATTR2 array 2 1

for size in 1 4 8 ; do
	test_result 88 --
	test_run libpdbg_attr_test / read ATTR2 array $size 1
done

test_result 0 <<EOF
0x00000000 
EOF
test_run libpdbg_attr_test / read ATTR4 array 4 1

for size in 1 2 8 ; do
	test_result 88 --
	test_run libpdbg_attr_test / read ATTR4 array $size 1
done

test_result 0 <<EOF
0x0000000000000000 
EOF
test_run libpdbg_attr_test / read ATTR8 array 8 1

for size in 1 2 4 ; do
	test_result 88 --
	test_run libpdbg_attr_test / read ATTR8 array $size 1
done

test_result 0 --
test_run libpdbg_attr_test / write ATTR1 array 1 1 0x11

test_result 0 <<EOF
0x11 
EOF
test_run libpdbg_attr_test / read ATTR1 array 1 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR2 array 1 2 0x11 0x22

test_result 0 <<EOF
0x11 0x22 
EOF
test_run libpdbg_attr_test / read ATTR2 array 1 2

test_result 0 --
test_run libpdbg_attr_test / write ATTR2 array 2 1 0x1212

test_result 0 <<EOF
0x1212 
EOF
test_run libpdbg_attr_test / read ATTR2 array 2 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR4 array 1 4 0x11 0x22 0x33 0x44

test_result 0 <<EOF
0x11 0x22 0x33 0x44 
EOF
test_run libpdbg_attr_test / read ATTR4 array 1 4

test_result 0 --
test_run libpdbg_attr_test / write ATTR4 array 2 2 0x1234 0x1234

test_result 0 <<EOF
0x1234 0x1234 
EOF
test_run libpdbg_attr_test / read ATTR4 array 2 2

test_result 0 --
test_run libpdbg_attr_test / write ATTR4 array 4 1 0x12345678

test_result 0 <<EOF
0x12345678 
EOF
test_run libpdbg_attr_test / read ATTR4 array 4 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR8 array 1 8 0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88

test_result 0 <<EOF
0x11 0x22 0x33 0x44 0x55 0x66 0x77 0x88 
EOF
test_run libpdbg_attr_test / read ATTR8 array 1 8

test_result 0 --
test_run libpdbg_attr_test / write ATTR8 array 2 4 0x1234 0x1234 0x1234 0x5678

test_result 0 <<EOF
0x1234 0x1234 0x1234 0x5678 
EOF
test_run libpdbg_attr_test / read ATTR8 array 2 4

test_result 0 --
test_run libpdbg_attr_test / write ATTR8 array 4 2 0x12345678 0x23456789

test_result 0 <<EOF
0x12345678 0x23456789 
EOF
test_run libpdbg_attr_test / read ATTR8 array 4 2

test_result 0 --
test_run libpdbg_attr_test / write ATTR8 array 8 1 0x1234567890abcdef

test_result 0 <<EOF
0x1234567890abcdef 
EOF
test_run libpdbg_attr_test / read ATTR8 array 8 1
