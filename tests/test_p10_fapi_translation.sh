#!/bin/sh

. $(dirname "$0")/driver.sh

test_group "p10 fapi translation tests"

export PDBG_BACKEND_DTB=bmc-kernel.dtb
export PDBG_DTB=p10.dtb

test_result 0 <<EOF
Testing /proc0/pib/chiplet@20000000/eq@0/fc@0/core@0  0
Testing /proc0/pib/chiplet@20000000/eq@0/fc@0/core@1  1
Testing /proc0/pib/chiplet@20000000/eq@0/fc@1/core@0  2
Testing /proc0/pib/chiplet@20000000/eq@0/fc@1/core@1  3
Testing /proc0/pib/chiplet@21000000/eq@1/fc@0/core@0  4
Testing /proc0/pib/chiplet@21000000/eq@1/fc@0/core@1  5
Testing /proc0/pib/chiplet@21000000/eq@1/fc@1/core@0  6
Testing /proc0/pib/chiplet@21000000/eq@1/fc@1/core@1  7
Testing /proc0/pib/chiplet@22000000/eq@2/fc@0/core@0  8
Testing /proc0/pib/chiplet@22000000/eq@2/fc@0/core@1  9
Testing /proc0/pib/chiplet@22000000/eq@2/fc@1/core@0  10
Testing /proc0/pib/chiplet@22000000/eq@2/fc@1/core@1  11
Testing /proc0/pib/chiplet@23000000/eq@3/fc@0/core@0  12
Testing /proc0/pib/chiplet@23000000/eq@3/fc@0/core@1  13
Testing /proc0/pib/chiplet@23000000/eq@3/fc@1/core@0  14
Testing /proc0/pib/chiplet@23000000/eq@3/fc@1/core@1  15
Testing /proc0/pib/chiplet@24000000/eq@4/fc@0/core@0  16
Testing /proc0/pib/chiplet@24000000/eq@4/fc@0/core@1  17
Testing /proc0/pib/chiplet@24000000/eq@4/fc@1/core@0  18
Testing /proc0/pib/chiplet@24000000/eq@4/fc@1/core@1  19
Testing /proc0/pib/chiplet@25000000/eq@5/fc@0/core@0  20
Testing /proc0/pib/chiplet@25000000/eq@5/fc@0/core@1  21
Testing /proc0/pib/chiplet@25000000/eq@5/fc@1/core@0  22
Testing /proc0/pib/chiplet@25000000/eq@5/fc@1/core@1  23
Testing /proc0/pib/chiplet@26000000/eq@6/fc@0/core@0  24
Testing /proc0/pib/chiplet@26000000/eq@6/fc@0/core@1  25
Testing /proc0/pib/chiplet@26000000/eq@6/fc@1/core@0  26
Testing /proc0/pib/chiplet@26000000/eq@6/fc@1/core@1  27
Testing /proc0/pib/chiplet@27000000/eq@7/fc@0/core@0  28
Testing /proc0/pib/chiplet@27000000/eq@7/fc@0/core@1  29
Testing /proc0/pib/chiplet@27000000/eq@7/fc@1/core@0  30
Testing /proc0/pib/chiplet@27000000/eq@7/fc@1/core@1  31
EOF

test_run libpdbg_p10_fapi_translation_test core


test_result 0 <<EOF
Testing /proc0/pib/chiplet@20000000/eq@0  0
Testing /proc0/pib/chiplet@21000000/eq@1  1
Testing /proc0/pib/chiplet@22000000/eq@2  2
Testing /proc0/pib/chiplet@23000000/eq@3  3
Testing /proc0/pib/chiplet@24000000/eq@4  4
Testing /proc0/pib/chiplet@25000000/eq@5  5
Testing /proc0/pib/chiplet@26000000/eq@6  6
Testing /proc0/pib/chiplet@27000000/eq@7  7
EOF

test_run libpdbg_p10_fapi_translation_test eq


test_result 0 <<EOF
Testing /proc0/pib/chiplet@8000000/pec@0  0
Testing /proc0/pib/chiplet@9000000/pec@1  1
EOF

test_run libpdbg_p10_fapi_translation_test pec


test_result 0 <<EOF
Testing /proc0/pib/chiplet@8000000/pec@0/phb@0  0
Testing /proc0/pib/chiplet@8000000/pec@0/phb@1  1
Testing /proc0/pib/chiplet@8000000/pec@0/phb@2  2
Testing /proc0/pib/chiplet@9000000/pec@1/phb@0  3
Testing /proc0/pib/chiplet@9000000/pec@1/phb@1  4
Testing /proc0/pib/chiplet@9000000/pec@1/phb@2  5
EOF

test_run libpdbg_p10_fapi_translation_test phb


test_result 0 <<EOF
Testing /proc0/pib/chiplet@c000000/mc@0/mi@0  0
Testing /proc0/pib/chiplet@d000000/mc@1/mi@1  1
Testing /proc0/pib/chiplet@e000000/mc@2/mi@2  2
Testing /proc0/pib/chiplet@f000000/mc@3/mi@3  3
EOF

test_run libpdbg_p10_fapi_translation_test mi


test_result 0 <<EOF
Testing /proc0/pib/chiplet@c000000/mc@0/mi@0/mcc@0  0
Testing /proc0/pib/chiplet@c000000/mc@0/mi@0/mcc@1  1
Testing /proc0/pib/chiplet@d000000/mc@1/mi@1/mcc@0  2
Testing /proc0/pib/chiplet@d000000/mc@1/mi@1/mcc@1  3
Testing /proc0/pib/chiplet@e000000/mc@2/mi@2/mcc@0  4
Testing /proc0/pib/chiplet@e000000/mc@2/mi@2/mcc@1  5
Testing /proc0/pib/chiplet@f000000/mc@3/mi@3/mcc@0  6
Testing /proc0/pib/chiplet@f000000/mc@3/mi@3/mcc@1  7
EOF

test_run libpdbg_p10_fapi_translation_test mcc


test_result 0 <<EOF
Testing /proc0/pib/chiplet@c000000/mc@0/omic@0  0
Testing /proc0/pib/chiplet@c000000/mc@0/omic@1  1
Testing /proc0/pib/chiplet@d000000/mc@1/omic@2  2
Testing /proc0/pib/chiplet@d000000/mc@1/omic@3  3
Testing /proc0/pib/chiplet@e000000/mc@2/omic@4  4
Testing /proc0/pib/chiplet@e000000/mc@2/omic@5  5
Testing /proc0/pib/chiplet@f000000/mc@3/omic@6  6
Testing /proc0/pib/chiplet@f000000/mc@3/omic@7  7
EOF

test_run libpdbg_p10_fapi_translation_test omic


test_result 0 <<EOF
Testing /proc0/pib/chiplet@1000000  1
Testing /proc0/pib/chiplet@2000000  2
Testing /proc0/pib/chiplet@3000000  3
Testing /proc0/pib/chiplet@8000000  8
Testing /proc0/pib/chiplet@9000000  9
Testing /proc0/pib/chiplet@c000000  12
Testing /proc0/pib/chiplet@d000000  13
Testing /proc0/pib/chiplet@e000000  14
Testing /proc0/pib/chiplet@f000000  15
Testing /proc0/pib/chiplet@10000000  16
Testing /proc0/pib/chiplet@11000000  17
Testing /proc0/pib/chiplet@12000000  18
Testing /proc0/pib/chiplet@13000000  19
Testing /proc0/pib/chiplet@18000000  24
Testing /proc0/pib/chiplet@19000000  25
Testing /proc0/pib/chiplet@1a000000  26
Testing /proc0/pib/chiplet@1b000000  27
Testing /proc0/pib/chiplet@1c000000  28
Testing /proc0/pib/chiplet@1d000000  29
Testing /proc0/pib/chiplet@1e000000  30
Testing /proc0/pib/chiplet@1f000000  31
Testing /proc0/pib/chiplet@20000000  32
Testing /proc0/pib/chiplet@21000000  33
Testing /proc0/pib/chiplet@22000000  34
Testing /proc0/pib/chiplet@23000000  35
Testing /proc0/pib/chiplet@24000000  36
Testing /proc0/pib/chiplet@25000000  37
Testing /proc0/pib/chiplet@26000000  38
Testing /proc0/pib/chiplet@27000000  39
EOF

test_run libpdbg_p10_fapi_translation_test chiplet


test_result 0 <<EOF
Testing /proc0/pib/chiplet@c000000/mc@0  0
Testing /proc0/pib/chiplet@d000000/mc@1  1
Testing /proc0/pib/chiplet@e000000/mc@2  2
Testing /proc0/pib/chiplet@f000000/mc@3  3
EOF

test_run libpdbg_p10_fapi_translation_test mc


test_result 0 <<EOF
Testing /proc0/pib/chiplet@2000000/nmmu@0  0
Testing /proc0/pib/chiplet@3000000/nmmu@1  1
EOF

test_run libpdbg_p10_fapi_translation_test nmmu


test_result 0 <<EOF
Testing /proc0/pib/chiplet@18000000/iohs@0  0
Testing /proc0/pib/chiplet@19000000/iohs@1  1
Testing /proc0/pib/chiplet@1a000000/iohs@2  2
Testing /proc0/pib/chiplet@1b000000/iohs@3  3
Testing /proc0/pib/chiplet@1c000000/iohs@4  4
Testing /proc0/pib/chiplet@1d000000/iohs@5  5
Testing /proc0/pib/chiplet@1e000000/iohs@6  6
Testing /proc0/pib/chiplet@1f000000/iohs@7  7
EOF

test_run libpdbg_p10_fapi_translation_test iohs


test_result 0 <<EOF
Testing /proc0/pib/chiplet@10000000/pauc@0/pau@0  0
Testing /proc0/pib/chiplet@11000000/pauc@1/pau@3  3
Testing /proc0/pib/chiplet@12000000/pauc@2/pau@4  4
Testing /proc0/pib/chiplet@12000000/pauc@2/pau@5  5
Testing /proc0/pib/chiplet@13000000/pauc@3/pau@6  6
Testing /proc0/pib/chiplet@13000000/pauc@3/pau@7  7
EOF

test_run libpdbg_p10_fapi_translation_test pau


test_result 0 <<EOF
Testing /proc0/pib/chiplet@10000000/pauc@0  0
Testing /proc0/pib/chiplet@11000000/pauc@1  1
Testing /proc0/pib/chiplet@12000000/pauc@2  2
Testing /proc0/pib/chiplet@13000000/pauc@3  3
EOF

test_run libpdbg_p10_fapi_translation_test pauc
