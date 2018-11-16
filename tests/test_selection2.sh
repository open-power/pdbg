#!/bin/sh

. $(dirname "$0")/driver.sh

test_group "path target selection tests"

arch=$(arch 2>/dev/null)

do_skip ()
{
	if [ "$arch" != "x86_64" ] ; then
		test_skip
	fi
}

test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p1: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p2: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p3: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p4: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p5: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p6: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p7: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P thread probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c1: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
    c3: Fake Core
        t0: Fake Thread
        t1: Fake Thread

EOF

do_skip
test_run pdbg -b fake -P pib0/thread probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p1: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p2: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p3: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p4: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p5: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p6: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
p7: Fake PIB
    c0: Fake Core
        t0: Fake Thread
        t1: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P core0/thread probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p1: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p2: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p3: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p4: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p5: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p6: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p7: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P thread0 probe


test_result 0 <<EOF
p0: Fake PIB
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P pib0/core2/thread probe


test_result 0 <<EOF
p0: Fake PIB
    c2: Fake Core
        t1: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P pib0/core2/thread1 probe


test_result 0 <<EOF
p1: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p2: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p3: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p5: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p6: Fake PIB
    c0: Fake Core
        t0: Fake Thread

EOF

do_skip
test_run pdbg -b fake -P pib[1-3,5,5-6]/core0/thread0 probe


test_result 0 <<EOF
p0: Fake PIB
EOF

do_skip
test_run pdbg -b fake -P pib0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
p1: Fake PIB
    c0: Fake Core
p2: Fake PIB
    c0: Fake Core
p3: Fake PIB
    c0: Fake Core
p4: Fake PIB
    c0: Fake Core
p5: Fake PIB
    c0: Fake Core
p6: Fake PIB
    c0: Fake Core
p7: Fake PIB
    c0: Fake Core
EOF

do_skip
test_run pdbg -b fake -P core0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p1: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p2: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p3: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p4: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p5: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p6: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p7: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P thread0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p1: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p2: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p3: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p4: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p5: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p6: Fake PIB
    c0: Fake Core
        t0: Fake Thread
p7: Fake PIB
    c0: Fake Core
        t0: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P core0/thread0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
    c1: Fake Core
        t0: Fake Thread
    c2: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P pib0/thread0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
EOF

do_skip
test_run pdbg -b fake -P pib0/core0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P pib0/core0/thread0 probe


test_result 0 <<EOF
p1: Fake PIB
    c1: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p3: Fake PIB
    c1: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p5: Fake PIB
    c1: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
p7: Fake PIB
    c1: Fake Core
        t0: Fake Thread
    c3: Fake Core
        t0: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P pib[1,3,5,7,9]/core[1,3,5]/thread[0,2] probe


test_result 0 <<EOF
p1: Fake PIB
    c1: Fake Core
        t1: Fake Thread
    c3: Fake Core
        t1: Fake Thread
p3: Fake PIB
    c1: Fake Core
        t1: Fake Thread
    c3: Fake Core
        t1: Fake Thread
p5: Fake PIB
    c1: Fake Core
        t1: Fake Thread
    c3: Fake Core
        t1: Fake Thread
p7: Fake PIB
    c1: Fake Core
        t1: Fake Thread
    c3: Fake Core
        t1: Fake Thread
EOF

do_skip
test_run pdbg -b fake -P pib[1,3]/core[1,3,5]/thread1 -P pib[5,7-9]/core[1,3,5]/thread1 probe

test_result 1 <<EOF
No valid targets found or specified. Try adding -p/-c/-t options to specify a target.
Alternatively run 'pdbg -a probe' to get a list of all valid targets
EOF

do_skip
test_run pdbg -b fake -P "fsi0/pib%d" probe
