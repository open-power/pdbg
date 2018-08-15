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

test_result 0 <<EOF

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -a probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -p0 -a probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -c0 -a probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -t0 -a probe


test_result 0 <<EOF
p0: Fake PIB
    c2: Fake Core
        t0: Fake Thread
        t1: Fake Thread

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -p0 -c2 -a probe


test_result 0 <<EOF
p0: Fake PIB
    c2: Fake Core
        t1: Fake Thread

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -p0 -c2 -t1 -a probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets

EOF

do_skip
test_run pdbg -b fake -p1-3,5,5-6 -c0 -t0 probe


test_result 0 <<EOF
p0: Fake PIB

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
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
p0: Fake PIB
    c0: Fake Core

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -c0 -p0 probe


test_result 0 <<EOF
p0: Fake PIB
    c0: Fake Core
        t0: Fake Thread

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -t0 -c0 -p0 probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
EOF

do_skip
test_run pdbg -b fake -p1,3,5,7,9 -c1,3,5 -t0,2 probe


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

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
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
