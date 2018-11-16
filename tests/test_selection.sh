#!/bin/sh

. $(dirname "$0")/driver.sh

test_group "target selection tests"

arch=$(arch 2>/dev/null)

do_skip ()
{
	if [ "$arch" != "x86_64" ] ; then
		test_skip
	fi
}

test_result 1 <<EOF
No valid targets found or specified. Try adding -p/-c/-t options to specify a target.
Alternatively run 'pdbg -a probe' to get a list of all valid targets
EOF

do_skip
test_run pdbg -b fake probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib1: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib2: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib3: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib4: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib5: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib6: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib7: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -a probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -p0 -a probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib1: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib2: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib3: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib4: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib5: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib6: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
    pib7: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -c0 -a probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib1: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib2: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib3: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib4: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib5: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib6: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib7: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -t0 -a probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core2: Fake Core (*)
            thread0: Fake Thread (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -p0 -c2 -a probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core2: Fake Core (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -p0 -c2 -t1 -a probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib1: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
    pib2: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
    pib3: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
    pib5: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
    pib6: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -p1-3,5,5-6 -c0 -t0 probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
EOF

do_skip
test_run pdbg -b fake -p0 probe


test_result 1 --
test_result_stderr <<EOF
No processor(s) selected
Use -p or -a to select processor(s)
EOF

do_skip
test_run pdbg -b fake -c0 probe


test_result 1 --
test_result_stderr <<EOF
No processor(s) selected
Use -p or -a to select processor(s)
EOF

do_skip
test_run pdbg -b fake -t0 probe


test_result 1 --
test_result_stderr <<EOF
No processor(s) selected
Use -p or -a to select processor(s)
EOF

do_skip
test_run pdbg -b fake -c0 -t0 probe


test_result 1 --
test_result_stderr <<EOF
No chip(s) selected
Use -c or -a to select chip(s)
EOF

do_skip
test_run pdbg -b fake -t0 -p0 probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core0: Fake Core (*)
EOF

do_skip
test_run pdbg -b fake -c0 -p0 probe


test_result 0 <<EOF
fsi0: Fake FSI (*)
    pib0: Fake PIB (*)
        core0: Fake Core (*)
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -t0 -c0 -p0 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib1: Fake PIB (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib3: Fake PIB (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib5: Fake PIB (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
    pib7: Fake PIB (*)
        core1: Fake Core (*)
            thread0: Fake Thread (*)
        core3: Fake Core (*)
            thread0: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -p1,3,5,7,9 -c1,3,5 -t0,2 probe


test_result 0 <<EOF
fsi0: Fake FSI
    pib1: Fake PIB (*)
        core1: Fake Core (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread1: Fake Thread (*)
    pib3: Fake PIB (*)
        core1: Fake Core (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread1: Fake Thread (*)
    pib5: Fake PIB (*)
        core1: Fake Core (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread1: Fake Thread (*)
    pib7: Fake PIB (*)
        core1: Fake Core (*)
            thread1: Fake Thread (*)
        core3: Fake Core (*)
            thread1: Fake Thread (*)
EOF

do_skip
test_run pdbg -b fake -p1,3 -p5 -p7-9 -c1 -c3 -c5 -t1 probe


test_result 1 --
test_result_stderr <<EOF
Value 100 larger than max 63
Failed to parse '-p 100'
EOF

do_skip
test_run pdbg -b fake -p100 probe


test_result 1 --
test_result_stderr <<EOF
Value 100 larger than max 23
Failed to parse '-c 100'
EOF

do_skip
test_run pdbg -b fake -c100 probe


test_result 1 --
test_result_stderr <<EOF
Value 100 larger than max 7
Failed to parse '-t 100'
EOF

do_skip
test_run pdbg -b fake -t100 probe
