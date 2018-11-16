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
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib1: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib2: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib3: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib4: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib5: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib6: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib7: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P thread probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib0/thread probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib1: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib2: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib3: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib4: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib5: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib6: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib7: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P core0/thread probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib1: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib2: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib3: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib4: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib5: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib6: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib7: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P thread0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core2: Fake Core
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib0/core2/thread probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core2: Fake Core
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib0/core2/thread1 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib1: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib2: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib3: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib5: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib6: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib[1-3,5,5-6]/core0/thread0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB (*)
EOF

do_skip
test_run pdbg -b fake -P pib0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core (*)
    pib1: Fake PIB
        core0: Fake Core (*)
    pib2: Fake PIB
        core0: Fake Core (*)
    pib3: Fake PIB
        core0: Fake Core (*)
    pib4: Fake PIB
        core0: Fake Core (*)
    pib5: Fake PIB
        core0: Fake Core (*)
    pib6: Fake PIB
        core0: Fake Core (*)
    pib7: Fake PIB
        core0: Fake Core (*)
EOF

do_skip
test_run pdbg -b fake -P core0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib1: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib2: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib3: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib4: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib5: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib6: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib7: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P thread0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib1: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib2: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib3: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib4: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib5: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib6: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
    pib7: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P core0/thread0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
        core1: Fake Core
            thread0: Fake Thread (*)
        core2: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib0/thread0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core (*)
EOF

do_skip
test_run pdbg -b fake -P pib0/core0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib0: Fake PIB
        core0: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib0/core0/thread0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib1: Fake PIB
        core1: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib3: Fake PIB
        core1: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib5: Fake PIB
        core1: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
    pib7: Fake PIB
        core1: Fake Core
            thread0: Fake Thread (*)
        core3: Fake Core
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib[1,3,5,7,9]/core[1,3,5]/thread[0,2] probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib1: Fake PIB
        core1: Fake Core
            thread1: Fake Thread (*)
        core3: Fake Core
            thread1: Fake Thread (*)
    pib3: Fake PIB
        core1: Fake Core
            thread1: Fake Thread (*)
        core3: Fake Core
            thread1: Fake Thread (*)
    pib5: Fake PIB
        core1: Fake Core
            thread1: Fake Thread (*)
        core3: Fake Core
            thread1: Fake Thread (*)
    pib7: Fake PIB
        core1: Fake Core
            thread1: Fake Thread (*)
        core3: Fake Core
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -P pib[1,3]/core[1,3,5]/thread1 -P pib[5,7-9]/core[1,3,5]/thread1 probe

test_result 1 <<EOF
No valid targets found or specified. Try adding -p/-c/-t options to specify a target.
Alternatively run 'pdbg -a probe' to get a list of all valid targets
EOF

do_skip
test_run pdbg -b fake -P "fsi0/pib%d" probe
