#!/bin/sh

. $(dirname "$0")/driver.sh

test_group "libpdbg property tests"

test_result 88 --
test_run libpdbg_prop_test /proc0/pib read ATTR0 int

test_result 88 --
test_run libpdbg_prop_test /proc1/pib read ATTR0 char

test_result 0 <<EOF
0x00c0ffee
EOF
test_run libpdbg_prop_test /proc0/pib read ATTR1 int

test_result 0 <<EOF
0x00c0ffee
EOF
test_run libpdbg_prop_test /proc1/pib read ATTR1 int

test_result 0 <<EOF
processor0<11>
EOF
test_run libpdbg_prop_test /proc0/pib read ATTR2 char

test_result 0 <<EOF
processor1<11>
EOF
test_run libpdbg_prop_test /proc1/pib read ATTR2 char

test_result 99 --
test_run libpdbg_prop_test /proc0/pib write ATTR1 int 0xdeadbeef

test_result 99 --
test_run libpdbg_prop_test /proc0/pib write ATTR2 char PROCESSOR0

cp fake.dtb fake-prop.dtb
test_cleanup rm -f fake-prop.dtb

cp fake-backend.dtb fake-backend-prop.dtb
test_cleanup rm -f fake-backend-prop.dtb

export PDBG_DTB=fake-prop.dtb
export PDBG_BACKEND_DTB=fake-backend-prop.dtb

test_result 0 --
test_run libpdbg_prop_test /proc1/pib write ATTR1 int 0xdeadbeef

test_result 0 --
test_run libpdbg_prop_test /proc1/pib write ATTR2 char PROCESSOR0

test_result 0 <<EOF
0xdeadbeef
EOF
test_run libpdbg_prop_test /proc1/pib read ATTR1 int

test_result 0 <<EOF
PROCESSOR0<11>
EOF
test_run libpdbg_prop_test /proc1/pib read ATTR2 char
