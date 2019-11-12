#!/bin/sh

. $(dirname "$0")/driver.sh

test_group "p9 fapi translation tests"

export PDBG_BACKEND_DTB=p9-kernel.dtb
export PDBG_DTB=p9.dtb

test_result 0 <<EOF
Testing /proc0/pib/chiplet@10000000/eq@0/ex@0/chiplet@20000000/core@0  0
Testing /proc0/pib/chiplet@10000000/eq@0/ex@0/chiplet@21000000/core@0  1
Testing /proc0/pib/chiplet@10000000/eq@0/ex@1/chiplet@22000000/core@0  2
Testing /proc0/pib/chiplet@10000000/eq@0/ex@1/chiplet@23000000/core@0  3
Testing /proc0/pib/chiplet@11000000/eq@1/ex@0/chiplet@24000000/core@0  4
Testing /proc0/pib/chiplet@11000000/eq@1/ex@0/chiplet@25000000/core@0  5
Testing /proc0/pib/chiplet@11000000/eq@1/ex@1/chiplet@26000000/core@0  6
Testing /proc0/pib/chiplet@11000000/eq@1/ex@1/chiplet@27000000/core@0  7
Testing /proc0/pib/chiplet@12000000/eq@2/ex@0/chiplet@28000000/core@0  8
Testing /proc0/pib/chiplet@12000000/eq@2/ex@0/chiplet@29000000/core@0  9
Testing /proc0/pib/chiplet@12000000/eq@2/ex@1/chiplet@2a000000/core@0  10
Testing /proc0/pib/chiplet@12000000/eq@2/ex@1/chiplet@2b000000/core@0  11
Testing /proc0/pib/chiplet@13000000/eq@3/ex@0/chiplet@2c000000/core@0  12
Testing /proc0/pib/chiplet@13000000/eq@3/ex@0/chiplet@2d000000/core@0  13
Testing /proc0/pib/chiplet@13000000/eq@3/ex@1/chiplet@2e000000/core@0  14
Testing /proc0/pib/chiplet@13000000/eq@3/ex@1/chiplet@2f000000/core@0  15
Testing /proc0/pib/chiplet@14000000/eq@4/ex@0/chiplet@30000000/core@0  16
Testing /proc0/pib/chiplet@14000000/eq@4/ex@0/chiplet@31000000/core@0  17
Testing /proc0/pib/chiplet@14000000/eq@4/ex@1/chiplet@32000000/core@0  18
Testing /proc0/pib/chiplet@14000000/eq@4/ex@1/chiplet@33000000/core@0  19
Testing /proc0/pib/chiplet@15000000/eq@5/ex@0/chiplet@34000000/core@0  20
Testing /proc0/pib/chiplet@15000000/eq@5/ex@0/chiplet@35000000/core@0  21
Testing /proc0/pib/chiplet@15000000/eq@5/ex@1/chiplet@36000000/core@0  22
Testing /proc0/pib/chiplet@15000000/eq@5/ex@1/chiplet@37000000/core@0  23
Testing /proc1/pib/chiplet@10000000/eq@0/ex@0/chiplet@20000000/core@0  0
Testing /proc1/pib/chiplet@10000000/eq@0/ex@0/chiplet@21000000/core@0  1
Testing /proc1/pib/chiplet@10000000/eq@0/ex@1/chiplet@22000000/core@0  2
Testing /proc1/pib/chiplet@10000000/eq@0/ex@1/chiplet@23000000/core@0  3
Testing /proc1/pib/chiplet@11000000/eq@1/ex@0/chiplet@24000000/core@0  4
Testing /proc1/pib/chiplet@11000000/eq@1/ex@0/chiplet@25000000/core@0  5
Testing /proc1/pib/chiplet@11000000/eq@1/ex@1/chiplet@26000000/core@0  6
Testing /proc1/pib/chiplet@11000000/eq@1/ex@1/chiplet@27000000/core@0  7
Testing /proc1/pib/chiplet@12000000/eq@2/ex@0/chiplet@28000000/core@0  8
Testing /proc1/pib/chiplet@12000000/eq@2/ex@0/chiplet@29000000/core@0  9
Testing /proc1/pib/chiplet@12000000/eq@2/ex@1/chiplet@2a000000/core@0  10
Testing /proc1/pib/chiplet@12000000/eq@2/ex@1/chiplet@2b000000/core@0  11
Testing /proc1/pib/chiplet@13000000/eq@3/ex@0/chiplet@2c000000/core@0  12
Testing /proc1/pib/chiplet@13000000/eq@3/ex@0/chiplet@2d000000/core@0  13
Testing /proc1/pib/chiplet@13000000/eq@3/ex@1/chiplet@2e000000/core@0  14
Testing /proc1/pib/chiplet@13000000/eq@3/ex@1/chiplet@2f000000/core@0  15
Testing /proc1/pib/chiplet@14000000/eq@4/ex@0/chiplet@30000000/core@0  16
Testing /proc1/pib/chiplet@14000000/eq@4/ex@0/chiplet@31000000/core@0  17
Testing /proc1/pib/chiplet@14000000/eq@4/ex@1/chiplet@32000000/core@0  18
Testing /proc1/pib/chiplet@14000000/eq@4/ex@1/chiplet@33000000/core@0  19
Testing /proc1/pib/chiplet@15000000/eq@5/ex@0/chiplet@34000000/core@0  20
Testing /proc1/pib/chiplet@15000000/eq@5/ex@0/chiplet@35000000/core@0  21
Testing /proc1/pib/chiplet@15000000/eq@5/ex@1/chiplet@36000000/core@0  22
Testing /proc1/pib/chiplet@15000000/eq@5/ex@1/chiplet@37000000/core@0  23
EOF

test_run libpdbg_p9_fapi_translation_test core


test_result 0 <<EOF
Testing /proc0/pib/chiplet@10000000/eq@0  0
Testing /proc0/pib/chiplet@11000000/eq@1  1
Testing /proc0/pib/chiplet@12000000/eq@2  2
Testing /proc0/pib/chiplet@13000000/eq@3  3
Testing /proc0/pib/chiplet@14000000/eq@4  4
Testing /proc0/pib/chiplet@15000000/eq@5  5
Testing /proc1/pib/chiplet@10000000/eq@0  0
Testing /proc1/pib/chiplet@11000000/eq@1  1
Testing /proc1/pib/chiplet@12000000/eq@2  2
Testing /proc1/pib/chiplet@13000000/eq@3  3
Testing /proc1/pib/chiplet@14000000/eq@4  4
Testing /proc1/pib/chiplet@15000000/eq@5  5
EOF

test_run libpdbg_p9_fapi_translation_test eq


test_result 0 <<EOF
Testing /proc0/pib/chiplet@10000000/eq@0/ex@0  0
Testing /proc0/pib/chiplet@10000000/eq@0/ex@1  1
Testing /proc0/pib/chiplet@11000000/eq@1/ex@0  0
Testing /proc0/pib/chiplet@11000000/eq@1/ex@1  1
Testing /proc0/pib/chiplet@12000000/eq@2/ex@0  0
Testing /proc0/pib/chiplet@12000000/eq@2/ex@1  1
Testing /proc0/pib/chiplet@13000000/eq@3/ex@0  0
Testing /proc0/pib/chiplet@13000000/eq@3/ex@1  1
Testing /proc0/pib/chiplet@14000000/eq@4/ex@0  0
Testing /proc0/pib/chiplet@14000000/eq@4/ex@1  1
Testing /proc0/pib/chiplet@15000000/eq@5/ex@0  0
Testing /proc0/pib/chiplet@15000000/eq@5/ex@1  1
Testing /proc1/pib/chiplet@10000000/eq@0/ex@0  0
Testing /proc1/pib/chiplet@10000000/eq@0/ex@1  1
Testing /proc1/pib/chiplet@11000000/eq@1/ex@0  0
Testing /proc1/pib/chiplet@11000000/eq@1/ex@1  1
Testing /proc1/pib/chiplet@12000000/eq@2/ex@0  0
Testing /proc1/pib/chiplet@12000000/eq@2/ex@1  1
Testing /proc1/pib/chiplet@13000000/eq@3/ex@0  0
Testing /proc1/pib/chiplet@13000000/eq@3/ex@1  1
Testing /proc1/pib/chiplet@14000000/eq@4/ex@0  0
Testing /proc1/pib/chiplet@14000000/eq@4/ex@1  1
Testing /proc1/pib/chiplet@15000000/eq@5/ex@0  0
Testing /proc1/pib/chiplet@15000000/eq@5/ex@1  1
EOF

test_run libpdbg_p9_fapi_translation_test ex


test_result 0 <<EOF
Testing /proc0/pib/chiplet@6000000/xbus@0  0
Testing /proc1/pib/chiplet@6000000/xbus@0  0
EOF

test_run libpdbg_p9_fapi_translation_test xbus


test_result 0 <<EOF
Testing /proc0/pib/chiplet@9000000/obus@0  0
Testing /proc0/pib/chiplet@c000000/obus@3  3
Testing /proc1/pib/chiplet@9000000/obus@0  0
Testing /proc1/pib/chiplet@c000000/obus@3  3
EOF

test_run libpdbg_p9_fapi_translation_test obus


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test nv


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test pec


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test phb


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test mi


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test dmi


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test mcc


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test omic


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test omi


test_result 0 <<EOF
Testing /proc0/pib/chiplet@3000000/n1/mcs1  1
Testing /proc0/pib/chiplet@5000000/n3/mcs0  0
Testing /proc1/pib/chiplet@3000000/n1/mcs1  1
Testing /proc1/pib/chiplet@5000000/n3/mcs0  0
EOF

test_run libpdbg_p9_fapi_translation_test mcs


test_result 0 <<EOF
Testing /proc0/pib/chiplet@7000000/mc@0/mca0  0
Testing /proc0/pib/chiplet@7000000/mc@0/mca1  1
Testing /proc0/pib/chiplet@7000000/mc@0/mca2  2
Testing /proc0/pib/chiplet@7000000/mc@0/mca3  3
Testing /proc0/pib/chiplet@8000000/mc@1/mca0  0
Testing /proc0/pib/chiplet@8000000/mc@1/mca1  1
Testing /proc0/pib/chiplet@8000000/mc@1/mca2  2
Testing /proc0/pib/chiplet@8000000/mc@1/mca3  3
Testing /proc1/pib/chiplet@7000000/mc@0/mca0  0
Testing /proc1/pib/chiplet@7000000/mc@0/mca1  1
Testing /proc1/pib/chiplet@7000000/mc@0/mca2  2
Testing /proc1/pib/chiplet@7000000/mc@0/mca3  3
Testing /proc1/pib/chiplet@8000000/mc@1/mca0  0
Testing /proc1/pib/chiplet@8000000/mc@1/mca1  1
Testing /proc1/pib/chiplet@8000000/mc@1/mca2  2
Testing /proc1/pib/chiplet@8000000/mc@1/mca3  3
EOF

test_run libpdbg_p9_fapi_translation_test mca


test_result 0 <<EOF
Testing /proc0/pib/chiplet@7000000/mc@0/mcbist  0
Testing /proc0/pib/chiplet@8000000/mc@1/mcbist  0
Testing /proc1/pib/chiplet@7000000/mc@0/mcbist  0
Testing /proc1/pib/chiplet@8000000/mc@1/mcbist  0
EOF

test_run libpdbg_p9_fapi_translation_test mcbist


test_result 0 <<EOF
Testing /proc0/pib/chiplet@1000000  1
Testing /proc0/pib/chiplet@2000000  2
Testing /proc0/pib/chiplet@3000000  3
Testing /proc0/pib/chiplet@4000000  4
Testing /proc0/pib/chiplet@5000000  5
Testing /proc0/pib/chiplet@6000000  6
Testing /proc0/pib/chiplet@7000000  7
Testing /proc0/pib/chiplet@8000000  8
Testing /proc0/pib/chiplet@9000000  9
Testing /proc0/pib/chiplet@c000000  12
Testing /proc0/pib/chiplet@d000000  13
Testing /proc0/pib/chiplet@e000000  14
Testing /proc0/pib/chiplet@f000000  15
Testing /proc0/pib/chiplet@10000000  16
Testing /proc0/pib/chiplet@10000000/eq@0/ex@0/chiplet@20000000  32
Testing /proc0/pib/chiplet@10000000/eq@0/ex@0/chiplet@21000000  33
Testing /proc0/pib/chiplet@10000000/eq@0/ex@1/chiplet@22000000  34
Testing /proc0/pib/chiplet@10000000/eq@0/ex@1/chiplet@23000000  35
Testing /proc0/pib/chiplet@11000000  17
Testing /proc0/pib/chiplet@11000000/eq@1/ex@0/chiplet@24000000  36
Testing /proc0/pib/chiplet@11000000/eq@1/ex@0/chiplet@25000000  37
Testing /proc0/pib/chiplet@11000000/eq@1/ex@1/chiplet@26000000  38
Testing /proc0/pib/chiplet@11000000/eq@1/ex@1/chiplet@27000000  39
Testing /proc0/pib/chiplet@12000000  18
Testing /proc0/pib/chiplet@12000000/eq@2/ex@0/chiplet@28000000  40
Testing /proc0/pib/chiplet@12000000/eq@2/ex@0/chiplet@29000000  41
Testing /proc0/pib/chiplet@12000000/eq@2/ex@1/chiplet@2a000000  42
Testing /proc0/pib/chiplet@12000000/eq@2/ex@1/chiplet@2b000000  43
Testing /proc0/pib/chiplet@13000000  19
Testing /proc0/pib/chiplet@13000000/eq@3/ex@0/chiplet@2c000000  44
Testing /proc0/pib/chiplet@13000000/eq@3/ex@0/chiplet@2d000000  45
Testing /proc0/pib/chiplet@13000000/eq@3/ex@1/chiplet@2e000000  46
Testing /proc0/pib/chiplet@13000000/eq@3/ex@1/chiplet@2f000000  47
Testing /proc0/pib/chiplet@14000000  20
Testing /proc0/pib/chiplet@14000000/eq@4/ex@0/chiplet@30000000  48
Testing /proc0/pib/chiplet@14000000/eq@4/ex@0/chiplet@31000000  49
Testing /proc0/pib/chiplet@14000000/eq@4/ex@1/chiplet@32000000  50
Testing /proc0/pib/chiplet@14000000/eq@4/ex@1/chiplet@33000000  51
Testing /proc0/pib/chiplet@15000000  21
Testing /proc0/pib/chiplet@15000000/eq@5/ex@0/chiplet@34000000  52
Testing /proc0/pib/chiplet@15000000/eq@5/ex@0/chiplet@35000000  53
Testing /proc0/pib/chiplet@15000000/eq@5/ex@1/chiplet@36000000  54
Testing /proc0/pib/chiplet@15000000/eq@5/ex@1/chiplet@37000000  55
Testing /proc1/pib/chiplet@1000000  1
Testing /proc1/pib/chiplet@2000000  2
Testing /proc1/pib/chiplet@3000000  3
Testing /proc1/pib/chiplet@4000000  4
Testing /proc1/pib/chiplet@5000000  5
Testing /proc1/pib/chiplet@6000000  6
Testing /proc1/pib/chiplet@7000000  7
Testing /proc1/pib/chiplet@8000000  8
Testing /proc1/pib/chiplet@9000000  9
Testing /proc1/pib/chiplet@c000000  12
Testing /proc1/pib/chiplet@d000000  13
Testing /proc1/pib/chiplet@e000000  14
Testing /proc1/pib/chiplet@f000000  15
Testing /proc1/pib/chiplet@10000000  16
Testing /proc1/pib/chiplet@10000000/eq@0/ex@0/chiplet@20000000  32
Testing /proc1/pib/chiplet@10000000/eq@0/ex@0/chiplet@21000000  33
Testing /proc1/pib/chiplet@10000000/eq@0/ex@1/chiplet@22000000  34
Testing /proc1/pib/chiplet@10000000/eq@0/ex@1/chiplet@23000000  35
Testing /proc1/pib/chiplet@11000000  17
Testing /proc1/pib/chiplet@11000000/eq@1/ex@0/chiplet@24000000  36
Testing /proc1/pib/chiplet@11000000/eq@1/ex@0/chiplet@25000000  37
Testing /proc1/pib/chiplet@11000000/eq@1/ex@1/chiplet@26000000  38
Testing /proc1/pib/chiplet@11000000/eq@1/ex@1/chiplet@27000000  39
Testing /proc1/pib/chiplet@12000000  18
Testing /proc1/pib/chiplet@12000000/eq@2/ex@0/chiplet@28000000  40
Testing /proc1/pib/chiplet@12000000/eq@2/ex@0/chiplet@29000000  41
Testing /proc1/pib/chiplet@12000000/eq@2/ex@1/chiplet@2a000000  42
Testing /proc1/pib/chiplet@12000000/eq@2/ex@1/chiplet@2b000000  43
Testing /proc1/pib/chiplet@13000000  19
Testing /proc1/pib/chiplet@13000000/eq@3/ex@0/chiplet@2c000000  44
Testing /proc1/pib/chiplet@13000000/eq@3/ex@0/chiplet@2d000000  45
Testing /proc1/pib/chiplet@13000000/eq@3/ex@1/chiplet@2e000000  46
Testing /proc1/pib/chiplet@13000000/eq@3/ex@1/chiplet@2f000000  47
Testing /proc1/pib/chiplet@14000000  20
Testing /proc1/pib/chiplet@14000000/eq@4/ex@0/chiplet@30000000  48
Testing /proc1/pib/chiplet@14000000/eq@4/ex@0/chiplet@31000000  49
Testing /proc1/pib/chiplet@14000000/eq@4/ex@1/chiplet@32000000  50
Testing /proc1/pib/chiplet@14000000/eq@4/ex@1/chiplet@33000000  51
Testing /proc1/pib/chiplet@15000000  21
Testing /proc1/pib/chiplet@15000000/eq@5/ex@0/chiplet@34000000  52
Testing /proc1/pib/chiplet@15000000/eq@5/ex@0/chiplet@35000000  53
Testing /proc1/pib/chiplet@15000000/eq@5/ex@1/chiplet@36000000  54
Testing /proc1/pib/chiplet@15000000/eq@5/ex@1/chiplet@37000000  55
EOF

test_run libpdbg_p9_fapi_translation_test chiplet


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test ppe


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test sbe


test_result 0 <<EOF
EOF

test_skip
test_run libpdbg_p9_fapi_translation_test capp


test_result 0 <<EOF
Testing /proc0/pib/chiplet@7000000/mc@0  0
Testing /proc0/pib/chiplet@8000000/mc@1  1
Testing /proc1/pib/chiplet@7000000/mc@0  0
Testing /proc1/pib/chiplet@8000000/mc@1  1
EOF

test_run libpdbg_p9_fapi_translation_test mc
