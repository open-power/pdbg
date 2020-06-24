#!/bin/sh

. $(dirname "$0")/driver.sh

test_group "tree tests for fake2.dts"

export PDBG_DTB="fake2.dtb"
export PDBG_BACKEND_DTB="fake2-backend.dtb"

test_result 0 <<EOF
/ (/)
   proc0 (/proc0)
      fsi@20000 (/proc0/fsi)
      pib@20100 (/proc0/pib)
         core@10010 (/proc0/pib/core@10010)
            thread@0 (/proc0/pib/core@10010/thread@0)
            thread@1 (/proc0/pib/core@10010/thread@1)
         core@10020 (/proc0/pib/core@10020)
            thread@0 (/proc0/pib/core@10020/thread@0)
            thread@1 (/proc0/pib/core@10020/thread@1)
         core@10030 (/proc0/pib/core@10030)
            thread@0 (/proc0/pib/core@10030/thread@0)
            thread@1 (/proc0/pib/core@10030/thread@1)
         core@10040 (/proc0/pib/core@10040)
            thread@0 (/proc0/pib/core@10040/thread@0)
            thread@1 (/proc0/pib/core@10040/thread@1)
   proc1 (/proc1)
      fsi@21000 (/proc1/fsi)
      pib@21100 (/proc1/pib)
         core@10010 (/proc1/pib/core@10010)
            thread@0 (/proc1/pib/core@10010/thread@0)
            thread@1 (/proc1/pib/core@10010/thread@1)
         core@10020 (/proc1/pib/core@10020)
            thread@0 (/proc1/pib/core@10020/thread@0)
            thread@1 (/proc1/pib/core@10020/thread@1)
         core@10030 (/proc1/pib/core@10030)
            thread@0 (/proc1/pib/core@10030/thread@0)
            thread@1 (/proc1/pib/core@10030/thread@1)
         core@10040 (/proc1/pib/core@10040)
            thread@0 (/proc1/pib/core@10040/thread@0)
            thread@1 (/proc1/pib/core@10040/thread@1)
   proc2 (/proc2)
      fsi@22000 (/proc2/fsi)
      pib@22100 (/proc2/pib)
         core@10010 (/proc2/pib/core@10010)
            thread@0 (/proc2/pib/core@10010/thread@0)
            thread@1 (/proc2/pib/core@10010/thread@1)
         core@10020 (/proc2/pib/core@10020)
            thread@0 (/proc2/pib/core@10020/thread@0)
            thread@1 (/proc2/pib/core@10020/thread@1)
         core@10030 (/proc2/pib/core@10030)
            thread@0 (/proc2/pib/core@10030/thread@0)
            thread@1 (/proc2/pib/core@10030/thread@1)
         core@10040 (/proc2/pib/core@10040)
            thread@0 (/proc2/pib/core@10040/thread@0)
            thread@1 (/proc2/pib/core@10040/thread@1)
   proc3 (/proc3)
      fsi@23000 (/proc3/fsi)
      pib@23100 (/proc3/pib)
         core@10010 (/proc3/pib/core@10010)
            thread@0 (/proc3/pib/core@10010/thread@0)
            thread@1 (/proc3/pib/core@10010/thread@1)
         core@10020 (/proc3/pib/core@10020)
            thread@0 (/proc3/pib/core@10020/thread@0)
            thread@1 (/proc3/pib/core@10020/thread@1)
         core@10030 (/proc3/pib/core@10030)
            thread@0 (/proc3/pib/core@10030/thread@0)
            thread@1 (/proc3/pib/core@10030/thread@1)
         core@10040 (/proc3/pib/core@10040)
            thread@0 (/proc3/pib/core@10040/thread@0)
            thread@1 (/proc3/pib/core@10040/thread@1)
   proc4 (/proc4)
      fsi@24000 (/proc4/fsi)
      pib@24100 (/proc4/pib)
         core@10010 (/proc4/pib/core@10010)
            thread@0 (/proc4/pib/core@10010/thread@0)
            thread@1 (/proc4/pib/core@10010/thread@1)
         core@10020 (/proc4/pib/core@10020)
            thread@0 (/proc4/pib/core@10020/thread@0)
            thread@1 (/proc4/pib/core@10020/thread@1)
         core@10030 (/proc4/pib/core@10030)
            thread@0 (/proc4/pib/core@10030/thread@0)
            thread@1 (/proc4/pib/core@10030/thread@1)
         core@10040 (/proc4/pib/core@10040)
            thread@0 (/proc4/pib/core@10040/thread@0)
            thread@1 (/proc4/pib/core@10040/thread@1)
   proc5 (/proc5)
      fsi@25000 (/proc5/fsi)
      pib@25100 (/proc5/pib)
         core@10010 (/proc5/pib/core@10010)
            thread@0 (/proc5/pib/core@10010/thread@0)
            thread@1 (/proc5/pib/core@10010/thread@1)
         core@10020 (/proc5/pib/core@10020)
            thread@0 (/proc5/pib/core@10020/thread@0)
            thread@1 (/proc5/pib/core@10020/thread@1)
         core@10030 (/proc5/pib/core@10030)
            thread@0 (/proc5/pib/core@10030/thread@0)
            thread@1 (/proc5/pib/core@10030/thread@1)
         core@10040 (/proc5/pib/core@10040)
            thread@0 (/proc5/pib/core@10040/thread@0)
            thread@1 (/proc5/pib/core@10040/thread@1)
   proc6 (/proc6)
      fsi@26000 (/proc6/fsi)
      pib@26100 (/proc6/pib)
         core@10010 (/proc6/pib/core@10010)
            thread@0 (/proc6/pib/core@10010/thread@0)
            thread@1 (/proc6/pib/core@10010/thread@1)
         core@10020 (/proc6/pib/core@10020)
            thread@0 (/proc6/pib/core@10020/thread@0)
            thread@1 (/proc6/pib/core@10020/thread@1)
         core@10030 (/proc6/pib/core@10030)
            thread@0 (/proc6/pib/core@10030/thread@0)
            thread@1 (/proc6/pib/core@10030/thread@1)
         core@10040 (/proc6/pib/core@10040)
            thread@0 (/proc6/pib/core@10040/thread@0)
            thread@1 (/proc6/pib/core@10040/thread@1)
   proc7 (/proc7)
      fsi@27000 (/proc7/fsi)
      pib@27100 (/proc7/pib)
         core@10010 (/proc7/pib/core@10010)
            thread@0 (/proc7/pib/core@10010/thread@0)
            thread@1 (/proc7/pib/core@10010/thread@1)
         core@10020 (/proc7/pib/core@10020)
            thread@0 (/proc7/pib/core@10020/thread@0)
            thread@1 (/proc7/pib/core@10020/thread@1)
         core@10030 (/proc7/pib/core@10030)
            thread@0 (/proc7/pib/core@10030/thread@0)
            thread@1 (/proc7/pib/core@10030/thread@1)
         core@10040 (/proc7/pib/core@10040)
            thread@0 (/proc7/pib/core@10040/thread@0)
            thread@1 (/proc7/pib/core@10040/thread@1)
EOF

test_run libpdbg_dtree_test tree system /


test_result 0 <<EOF
/ (/)
   fsi@20000 (/proc0/fsi)
      pib@20100 (/proc0/pib)
         core@10010 (/proc0/pib/core@10010)
            thread@0 (/proc0/pib/core@10010/thread@0)
            thread@1 (/proc0/pib/core@10010/thread@1)
         core@10020 (/proc0/pib/core@10020)
            thread@0 (/proc0/pib/core@10020/thread@0)
            thread@1 (/proc0/pib/core@10020/thread@1)
         core@10030 (/proc0/pib/core@10030)
            thread@0 (/proc0/pib/core@10030/thread@0)
            thread@1 (/proc0/pib/core@10030/thread@1)
         core@10040 (/proc0/pib/core@10040)
            thread@0 (/proc0/pib/core@10040/thread@0)
            thread@1 (/proc0/pib/core@10040/thread@1)
   fsi@21000 (/proc1/fsi)
      pib@21100 (/proc1/pib)
         core@10010 (/proc1/pib/core@10010)
            thread@0 (/proc1/pib/core@10010/thread@0)
            thread@1 (/proc1/pib/core@10010/thread@1)
         core@10020 (/proc1/pib/core@10020)
            thread@0 (/proc1/pib/core@10020/thread@0)
            thread@1 (/proc1/pib/core@10020/thread@1)
         core@10030 (/proc1/pib/core@10030)
            thread@0 (/proc1/pib/core@10030/thread@0)
            thread@1 (/proc1/pib/core@10030/thread@1)
         core@10040 (/proc1/pib/core@10040)
            thread@0 (/proc1/pib/core@10040/thread@0)
            thread@1 (/proc1/pib/core@10040/thread@1)
   fsi@22000 (/proc2/fsi)
      pib@22100 (/proc2/pib)
         core@10010 (/proc2/pib/core@10010)
            thread@0 (/proc2/pib/core@10010/thread@0)
            thread@1 (/proc2/pib/core@10010/thread@1)
         core@10020 (/proc2/pib/core@10020)
            thread@0 (/proc2/pib/core@10020/thread@0)
            thread@1 (/proc2/pib/core@10020/thread@1)
         core@10030 (/proc2/pib/core@10030)
            thread@0 (/proc2/pib/core@10030/thread@0)
            thread@1 (/proc2/pib/core@10030/thread@1)
         core@10040 (/proc2/pib/core@10040)
            thread@0 (/proc2/pib/core@10040/thread@0)
            thread@1 (/proc2/pib/core@10040/thread@1)
   fsi@23000 (/proc3/fsi)
      pib@23100 (/proc3/pib)
         core@10010 (/proc3/pib/core@10010)
            thread@0 (/proc3/pib/core@10010/thread@0)
            thread@1 (/proc3/pib/core@10010/thread@1)
         core@10020 (/proc3/pib/core@10020)
            thread@0 (/proc3/pib/core@10020/thread@0)
            thread@1 (/proc3/pib/core@10020/thread@1)
         core@10030 (/proc3/pib/core@10030)
            thread@0 (/proc3/pib/core@10030/thread@0)
            thread@1 (/proc3/pib/core@10030/thread@1)
         core@10040 (/proc3/pib/core@10040)
            thread@0 (/proc3/pib/core@10040/thread@0)
            thread@1 (/proc3/pib/core@10040/thread@1)
   fsi@24000 (/proc4/fsi)
      pib@24100 (/proc4/pib)
         core@10010 (/proc4/pib/core@10010)
            thread@0 (/proc4/pib/core@10010/thread@0)
            thread@1 (/proc4/pib/core@10010/thread@1)
         core@10020 (/proc4/pib/core@10020)
            thread@0 (/proc4/pib/core@10020/thread@0)
            thread@1 (/proc4/pib/core@10020/thread@1)
         core@10030 (/proc4/pib/core@10030)
            thread@0 (/proc4/pib/core@10030/thread@0)
            thread@1 (/proc4/pib/core@10030/thread@1)
         core@10040 (/proc4/pib/core@10040)
            thread@0 (/proc4/pib/core@10040/thread@0)
            thread@1 (/proc4/pib/core@10040/thread@1)
   fsi@25000 (/proc5/fsi)
      pib@25100 (/proc5/pib)
         core@10010 (/proc5/pib/core@10010)
            thread@0 (/proc5/pib/core@10010/thread@0)
            thread@1 (/proc5/pib/core@10010/thread@1)
         core@10020 (/proc5/pib/core@10020)
            thread@0 (/proc5/pib/core@10020/thread@0)
            thread@1 (/proc5/pib/core@10020/thread@1)
         core@10030 (/proc5/pib/core@10030)
            thread@0 (/proc5/pib/core@10030/thread@0)
            thread@1 (/proc5/pib/core@10030/thread@1)
         core@10040 (/proc5/pib/core@10040)
            thread@0 (/proc5/pib/core@10040/thread@0)
            thread@1 (/proc5/pib/core@10040/thread@1)
   fsi@26000 (/proc6/fsi)
      pib@26100 (/proc6/pib)
         core@10010 (/proc6/pib/core@10010)
            thread@0 (/proc6/pib/core@10010/thread@0)
            thread@1 (/proc6/pib/core@10010/thread@1)
         core@10020 (/proc6/pib/core@10020)
            thread@0 (/proc6/pib/core@10020/thread@0)
            thread@1 (/proc6/pib/core@10020/thread@1)
         core@10030 (/proc6/pib/core@10030)
            thread@0 (/proc6/pib/core@10030/thread@0)
            thread@1 (/proc6/pib/core@10030/thread@1)
         core@10040 (/proc6/pib/core@10040)
            thread@0 (/proc6/pib/core@10040/thread@0)
            thread@1 (/proc6/pib/core@10040/thread@1)
   fsi@27000 (/proc7/fsi)
      pib@27100 (/proc7/pib)
         core@10010 (/proc7/pib/core@10010)
            thread@0 (/proc7/pib/core@10010/thread@0)
            thread@1 (/proc7/pib/core@10010/thread@1)
         core@10020 (/proc7/pib/core@10020)
            thread@0 (/proc7/pib/core@10020/thread@0)
            thread@1 (/proc7/pib/core@10020/thread@1)
         core@10030 (/proc7/pib/core@10030)
            thread@0 (/proc7/pib/core@10030/thread@0)
            thread@1 (/proc7/pib/core@10030/thread@1)
         core@10040 (/proc7/pib/core@10040)
            thread@0 (/proc7/pib/core@10040/thread@0)
            thread@1 (/proc7/pib/core@10040/thread@1)
   proc0 (/proc0)
   proc1 (/proc1)
   proc2 (/proc2)
   proc3 (/proc3)
   proc4 (/proc4)
   proc5 (/proc5)
   proc6 (/proc6)
   proc7 (/proc7)
EOF

test_run libpdbg_dtree_test tree backend /


test_result 0 <<EOF
proc1 (/proc1)
   fsi@21000 (/proc1/fsi)
   pib@21100 (/proc1/pib)
      core@10010 (/proc1/pib/core@10010)
         thread@0 (/proc1/pib/core@10010/thread@0)
         thread@1 (/proc1/pib/core@10010/thread@1)
      core@10020 (/proc1/pib/core@10020)
         thread@0 (/proc1/pib/core@10020/thread@0)
         thread@1 (/proc1/pib/core@10020/thread@1)
      core@10030 (/proc1/pib/core@10030)
         thread@0 (/proc1/pib/core@10030/thread@0)
         thread@1 (/proc1/pib/core@10030/thread@1)
      core@10040 (/proc1/pib/core@10040)
         thread@0 (/proc1/pib/core@10040/thread@0)
         thread@1 (/proc1/pib/core@10040/thread@1)
EOF

test_run libpdbg_dtree_test tree system /proc1


test_result 0 <<EOF
proc1 (/proc1)
EOF

test_run libpdbg_dtree_test tree backend /proc1


test_result 0 <<EOF
fsi@20000 (/proc0/fsi)
EOF

test_run libpdbg_dtree_test tree system /proc0/fsi


test_result 0 <<EOF
fsi@20000 (/proc0/fsi)
   pib@20100 (/proc0/pib)
      core@10010 (/proc0/pib/core@10010)
         thread@0 (/proc0/pib/core@10010/thread@0)
         thread@1 (/proc0/pib/core@10010/thread@1)
      core@10020 (/proc0/pib/core@10020)
         thread@0 (/proc0/pib/core@10020/thread@0)
         thread@1 (/proc0/pib/core@10020/thread@1)
      core@10030 (/proc0/pib/core@10030)
         thread@0 (/proc0/pib/core@10030/thread@0)
         thread@1 (/proc0/pib/core@10030/thread@1)
      core@10040 (/proc0/pib/core@10040)
         thread@0 (/proc0/pib/core@10040/thread@0)
         thread@1 (/proc0/pib/core@10040/thread@1)
EOF

test_run libpdbg_dtree_test tree backend /proc0/fsi


test_result 0 <<EOF
pib@22100 (/proc2/pib)
   core@10010 (/proc2/pib/core@10010)
      thread@0 (/proc2/pib/core@10010/thread@0)
      thread@1 (/proc2/pib/core@10010/thread@1)
   core@10020 (/proc2/pib/core@10020)
      thread@0 (/proc2/pib/core@10020/thread@0)
      thread@1 (/proc2/pib/core@10020/thread@1)
   core@10030 (/proc2/pib/core@10030)
      thread@0 (/proc2/pib/core@10030/thread@0)
      thread@1 (/proc2/pib/core@10030/thread@1)
   core@10040 (/proc2/pib/core@10040)
      thread@0 (/proc2/pib/core@10040/thread@0)
      thread@1 (/proc2/pib/core@10040/thread@1)
EOF

test_run libpdbg_dtree_test tree system /proc2/pib


test_result 0 <<EOF
pib@22100 (/proc2/pib)
   core@10010 (/proc2/pib/core@10010)
      thread@0 (/proc2/pib/core@10010/thread@0)
      thread@1 (/proc2/pib/core@10010/thread@1)
   core@10020 (/proc2/pib/core@10020)
      thread@0 (/proc2/pib/core@10020/thread@0)
      thread@1 (/proc2/pib/core@10020/thread@1)
   core@10030 (/proc2/pib/core@10030)
      thread@0 (/proc2/pib/core@10030/thread@0)
      thread@1 (/proc2/pib/core@10030/thread@1)
   core@10040 (/proc2/pib/core@10040)
      thread@0 (/proc2/pib/core@10040/thread@0)
      thread@1 (/proc2/pib/core@10040/thread@1)
EOF

test_run libpdbg_dtree_test tree backend /proc2/pib


test_result 0 <<EOF
thread@1
  core@10040
    pib@27100
      proc7
        /
EOF

test_run libpdbg_dtree_test rtree system /proc7/pib/core@10040/thread@1


test_result 0 <<EOF
thread@1
  core@10040
    pib@27100
      proc7
        /
EOF

test_run libpdbg_dtree_test rtree system /fsi@27000/pib@27100/core@10040/thread@1


test_result 0 <<EOF
thread@1
  core@10040
    pib@27100
      fsi@27000
        /
EOF

test_run libpdbg_dtree_test rtree backend /proc7/pib/core@10040/thread@1


test_result 0 <<EOF
thread@1
  core@10040
    pib@27100
      fsi@27000
        /
EOF

test_run libpdbg_dtree_test rtree backend /fsi@27000/pib@27100/core@10040/thread@1
