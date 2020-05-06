#!/bin/sh

. $(dirname "$0")/driver.sh

FAKE_DTB=fake.dtb
TEST_DTB=fake-attr.dtb

export PDBG_DTB=$TEST_DTB

test_setup cp $FAKE_DTB $TEST_DTB
test_cleanup rm -f $TEST_DTB

test_group "libpdbg packed attribute tests"

for size in 1 2 4 8 ; do
	test_result 88 --
	test_run libpdbg_attr_test / read ATTR16 packed $size 1
done

test_result 0 --
test_run libpdbg_attr_test / write ATTR16 packed 4444 1 0x1111 0x2222 0x3333 0x4444

test_result 0 <<EOF
0x00001111 0x00002222 0x00003333 0x00004444 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 4444 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR16 packed 8 2 0x12345678 0x23456789

test_result 0 <<EOF
0x0000000012345678 0x0000000023456789 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 8 2

test_result 0 --
test_run libpdbg_attr_test / write ATTR16 packed 844 1 0x11111111 0x22222222 0x33333333

test_result 0 <<EOF
0x0000000011111111 0x22222222 0x33333333 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 844 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR16 packed 8422 1 0x11111111 0x22222222 0x3333 0x4444

test_result 0 <<EOF
0x0000000011111111 0x22222222 0x3333 0x4444 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 8422 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR16 packed 12481 1 0x11 0x22 0x33 0x44 0x55

test_result 0 <<EOF
0x11 0x0022 0x00000033 0x0000000000000044 0x55 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 12481 1

test_result 0 --
test_run libpdbg_attr_test / write ATTR16 packed 8 2 0x1122334455667788 0x9900aabbccddeeff

test_result 0 <<EOF
0x1122334455667788 0x9900aabbccddeeff 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 8 2

test_result 0 <<EOF
0x11223344 0x55667788 0x9900aabb 0xccddeeff 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 44 2

test_result 0 <<EOF
0x1122 0x3344 0x5566 0x7788 0x9900 0xaabb 0xccdd 0xeeff 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 22 4

test_result 0 <<EOF
0x1122 0x3344556677889900 0xaabbccdd 0xeeff 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 2842 1

test_result 0 <<EOF
0x11 0x2233 0x44 0x55 0x6677 0x88 0x99 0x00aa 0xbb 0xcc 0xddee 0xff 
EOF
test_run libpdbg_attr_test / read ATTR16 packed 121 4
