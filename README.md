# PDBG

pdbg is a simple application to allow debugging of the host POWER
processors from the BMC. It works in a similar way to JTAG programmers
for embedded system development in that it allows you to access GPRs,
SPRs and system memory.

A remote gdb sever is under development to allow integration with
standard debugging tools.

## Building

The output of autoconf is not included in the git tree so it needs to be
generated using autoreconf. This can be done by running `./bootstrap.sh` in the
top level directory. Static linking is supported and can be performed by adding
`CFLAGS=-static` to the command line passed to configure.

### Cross compiling for BMC (ARM)

```
apt-get install gcc-arm-linux-gnueabi
./bootstrap.sh
./configure --host=arm-linux-gnueabi CFLAGS="-static"
make
rsync pdbg root@bmc:/usr/local/bin
```

## Usage

Several backends are supported depending on which system you are using and are
selected using the `-b` option:

POWER8 Backends:

- i2c (default): Uses an i2c connection between BMC and host processor
- fsi: Uses a bit-banging GPIO backend which accesses BMC registers directly via
  /dev/mem/. Requires `-d p8` to specify you are running on a POWER8 system.

POWER9 Backends:

- kernel (default): Uses the in kernel OpenFSI driver provided by OpenBMC
- fsi: Uses a bit-banging GPIO backend which accesses BMC registers directly via
  /dev/mem. Requiers `-d p9w/p9r/p9z` as appropriate for the system.
- sbefifo: Uses the in kernel OpenFSI & SBEFIFO drivers provided by OpenBMC

When using the fsi backend POWER8 AMI based BMC's must first be put into debug
mode to allow access to the relevant GPIOs:

`ipmitool -H <host> -U <username> -P <password> raw 0x3a 0x01`

On POWER9 when using the fsi backend it is also a good idea to put the BMC into
debug mode to prevent conflicts with the OpenFSI driver. On the BMC run:

`systemctl start fsi-disable.service && systemctl stop
host-failure-reboots@0.service`

Usage is straight forward. Note that if the binary is not statically linked all
commands need to be prefixed with LD\_LIBRARY\_PATH=<path to libpdbg> in
addition to the arguments for selecting a backend.

### Target Selection

pdbg has commands that operate on specific hardware unit(s) inside the
POWER processor.  To select appropriate hardware unit (commonly referred
as **target**), pdbg provides two different mechanisms.

#### Select processor(s) / Core(s) / Thread(s) using -p/-c/-t/-a/-l

Many commands typically operate on hardware thread(s) or CPU(s)
as identified by Linux.

 - all threads (`-a`)
 - core 0 of processor 0 (`-p0 -c0`)
 - all threads on processor 0  (`-p0 -a`)
 - all threads on core 1 of processor 0 (`-p0 -c1 -a`)
 - thread 2 on core 1 of processor 0 (`-p0 -c1 -t2`)
 - thread 0 on all cores of processor 0 (`-p0 -t0 -a`)
 - threads 1,2,3,4 on cores 1,3,5 of processor 1 (`-p1 -c1,3,5 -t1-4`)
 - CPUs 15 and 17 as identified by Linux (`-l15,17`)

Note: `-l` option is only available when running `pdbg` on the host.

#### Select targets based on path using -P

To select any target in a device tree, it can be specified using `-P`.
The -P option takes path specification as an argument.  This path specification
is constructed using the *class* names of targets present in a device tree.

Some of the targets currently available for selection are:

 - `pib`
 - `core`
 - `thread`
 - `adu`
 - `fsi`
 - `chiplet`

Path specification can be either an individual target or a *path* constructed
using more than one targets.

 - all threads (`-P thread`)
 - core 0 of processor 0 (`-P pib0/core0`)
 - all threads on processor 0 (`-P pib0/thread`)
 - all threads on core 1 of processor 0 (`-P pib0/core1/thread`)
 - thread 2 on core 1 of processor 0 (`-P pib0/core1/thread2`)
 - thread 0 on all cores of processor 0 (`-P pib0/thread0`)
 - threads 1,2,3,4 on cores 1,3,5 of processor 1 (`-P pib1/core[1,3,5]/thread[1-4]`)
 - chiplet at address 21000000 (-P `chiplet@21000000`)
 - all adus (`-P adu`)
 - First FSI (`-P fsi0`)

## Examples

```
$ ./pdbg --help
Usage: ./pdbg [options] command ...

 Options:
        -p, --processor=processor-id
        -c, --chip=chiplet-id
        -t, --thread=thread
        -a, --all
                Run command on all possible processors/chips/threads (default)
        -b, --backend=backend
                fsi:    An experimental backend that uses
                        bit-banging to access the host processor
                        via the FSI bus.
                i2c:    The P8 only backend which goes via I2C.
                kernel: The default backend which goes the kernel FSI driver.
        -d, --device=backend device
                For I2C the device node used by the backend to access the bus.
                For FSI the system board type, one of p8 or p9w
                Defaults to /dev/i2c4 for I2C
        -s, --slave-address=backend device address
                Device slave address to use for the backend. Not used by FSI
                and defaults to 0x50 for I2C
        -V, --version
        -h, --help

 Commands:
        getcfam <address>
        putcfam <address> <value> [<mask>]
        getscom <address>
        putscom <address> <value> [<mask>]
        getmem <address> <count>
        putmem <address>
        getvmem <virtual address>
        getgpr <gpr>
        putgpr <gpr> <value>
        getnia
        putnia <value>
        getspr <spr>
        putspr <spr> <value>
        start
        step <count>
        stop
        threadstatus
        probe
```
### Probe chip/processor/thread numbers
```
$ ./pdbg -a probe
proc0: Processor Module
    fsi0: Kernel based FSI master (*)
    pib0: Kernel based FSI SCOM (*)
        chiplet16: POWER9 Chiplet
            eq0: POWER9 eq
                ex0: POWER9 ex
                    chiplet32: POWER9 Chiplet
                        core0: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet33: POWER9 Chiplet
                        core1: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                ex1: POWER9 ex
                    chiplet34: POWER9 Chiplet
                        core2: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet35: POWER9 Chiplet
                        core3: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
        chiplet17: POWER9 Chiplet
            eq1: POWER9 eq
                ex0: POWER9 ex
                    chiplet36: POWER9 Chiplet
                        core4: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet37: POWER9 Chiplet
                        core5: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                ex1: POWER9 ex
        chiplet18: POWER9 Chiplet
            eq2: POWER9 eq
                ex0: POWER9 ex
                    chiplet40: POWER9 Chiplet
                        core8: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet41: POWER9 Chiplet
                        core9: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                ex1: POWER9 ex
        chiplet19: POWER9 Chiplet
            eq3: POWER9 eq
                ex0: POWER9 ex
                    chiplet44: POWER9 Chiplet
                        core12: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet45: POWER9 Chiplet
                        core13: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                ex1: POWER9 ex
                    chiplet46: POWER9 Chiplet
                        core14: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet47: POWER9 Chiplet
                        core15: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
        chiplet20: POWER9 Chiplet
            eq4: POWER9 eq
                ex0: POWER9 ex
                    chiplet48: POWER9 Chiplet
                        core16: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet49: POWER9 Chiplet
                        core17: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                ex1: POWER9 ex
        chiplet21: POWER9 Chiplet
            eq5: POWER9 eq
                ex0: POWER9 ex
                ex1: POWER9 ex
                    chiplet54: POWER9 Chiplet
                        core22: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
                    chiplet55: POWER9 Chiplet
                        core23: POWER9 Core (*)
                            thread0: POWER9 Thread (*)
                            thread1: POWER9 Thread (*)
                            thread2: POWER9 Thread (*)
                            thread3: POWER9 Thread (*)
proc1: Processor Module
proc2: Processor Module
proc3: Processor Module
proc4: Processor Module
proc5: Processor Module
proc6: Processor Module
proc7: Processor Module

Note that only selected targets (marked with *) and targets in the
hierarchy of the selected targets will be shown above. If none are shown
try adding '-a' to select all targets.
```

Core-IDs are core/chip numbers which should be passed as arguments to `-c`
when performing operations such as getgpr that operate on particular cores.
Processor-IDs should be passed as arguments to `-p` to operate on different
processor chips. Specifying no targets is an error and will result in the
following error message:

```
Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
```

If the above error occurs even though targets were specified it means the
specified targets were not found when probing the system.

### Read SCOM register
```
$ ./pdbg -P pib getscom 0xf000f
p0: 0x00000000000f000f = 0x222d104900008040 (/proc0/pib)
p1: 0x00000000000f000f = 0x222d104900008040 (/proc1/pib)
```

### Write SCOM register on secondary processor
`$ ./pdbg -P pib1 putscom 0x8013c02 0x0`

### Get thread status
```
$ ./pdbg -a threadstatus

p0t: 0 1 2 3
c22: A A A A
c21: A A A A
c20: A A A A
c19: A A A A
c15: A A A A
c14: A A A A
c07: A A A A
c05: A A A A

p1t: 0 1 2 3
c23: A A A A
c22: A A A A
c21: A A A A
c20: A A A A
c19: A A A A
c18: A A A A
c17: A A A A
c16: A A A A
```

### Stop thread execution on thread 0-4 of processor 0 core/chip 22
Reading thread register values requires all threads on a given core to be in the
quiesced state.
```
$ ./pdbg -p0 -c22 -t0 -t1 -t2 -t3 stop
$ ./pdbg -p0 -c22 -t0 -t1 -t2 -t3 threadstatus

p0t: 0 1 2 3
c22: Q Q Q Q
```

### Read GPR on thread 0 of processor 0 core/chip 22
```
$ ./pdbg -p0 -c22 -t0 getgpr 2
p0:c22:t0:gpr02: 0xc000000000f09900
```

### Read SPR 8 (LR) on thread 0 of processor 0 core/chip 22
```
$ ./pdbg -p0 -c22 -t0 getspr 8
p0:c22:t0:spr008: 0xc0000000008a97f0
```

### Restart thread 0-4 execution on processor 0 core/chip 22
```
./pdbg -p0 -c22 -t0 -t1 -t2 -t3 start
./pdbg -p0 -c22 -t0 -t1 -t2 -t3 threadstatus

p0t: 0 1 2 3
c22: A A A A
```

### Write to memory through processor 1
```
$ echo hello | sudo ./pdbg -p 1 putmem 0x250000001
Wrote 6 bytes starting at 0x0000000250000001
```

### Read 6 bytes from memory through processor 1
```
$ sudo ./pdbg -p 1 getmem 0x250000001 6 | hexdump -C
0x0000000250000000:    68 65 6c 6c 6f 0a

$ sudo ./pdbg -p 1 getmem 0x250000001 6 --raw | hexdump -C
00000000  68 65 6c 6c 6f 0a                                 |hello.|
00000006
```

### Write to cache-inhibited memory through processor 1
```
$ echo hello | sudo ./pdbg -p 1 putmem --ci 0x3fe88202
Wrote 6 bytes starting at 0x000000003fe88202
```

### Read from cache-inhibited memory through processor 1
```
$ sudo ./pdbg -p 1 getmem --ci 0x3fe88202 6 --raw | hexdump -C
00000000  68 65 6c 6c 6f 0a                                 |hello.|
00000006
```

### Read 4 bytes from the hardware RNG
```
$ lsprop /proc/device-tree/hwrng@3ffff40000000/
ibm,chip-id      00000000
compatible       "ibm,power-rng"
reg              0003ffff 40000000 00000000 00001000
phandle          100003bd (268436413)
name             "hwrng"

$ sudo ./pdbg -p 0  getmem --ci 0x0003ffff40000000 4 --raw |hexdump -C
00000000  01 c0 d1 79                                       |...y|
00000004
$  sudo ./pdbg -p 0  getmem --ci 0x0003ffff40000000 4 --raw |hexdump -C
00000000  77 9b ab ce                                       |w...|
00000004
$  sudo ./pdbg -p 0  getmem --ci 0x0003ffff40000000 4 --raw |hexdump -C
00000000  66 8d fb 42                                       |f..B|
00000004
$  sudo ./pdbg -p 0  getmem --ci 0x0003ffff40000000 4 --raw |hexdump -C
00000000  fa 9b e3 44                                       |...D|
00000004
```

### Hardware Trace Macro (HTM)

Exploitation of HTM is limited to POWER8 Core from the powerpc host.

#### Prerequisites

Core HTM on POWER8 needs to run SMT1 and no power save, so you need to
run this first:
```
ppc64_cpu --smt=1
for i in /sys/devices/system/cpu/cpu*/cpuidle/state*/disable;do echo 1 > $i;done
```
Also, using HTM requires a kernel built with both `CONFIG_PPC_MEMTRACE=y`
(v4.14) and `CONFIG_SCOM_DEBUGFS=y`. debugfs should be mounted at
`/sys/kernel/debug`. Ubuntu 18.04 has this by default.

#### How to run HTM

pdbg provides a `htm` command with a variety of sub-commands. The most
useful command is `record` which will start the trace, wait for buffer
to fill (~1 sec), stop and then dump the trace to a file (~5 sec). eg.
```
pdbg -l 0 htm core record
```
pdbg -l allows users to specify CPUs using the same addressing as
scheme as taskset -c. This can be useful for tracing workloads. eg.
```
taskset -c 0 myworkload
sleep 1
pdbg -l 0 htm core record
```
There are also low level htm commands which can also be used:
 - `start` will configure the hardware and start tracing in wrapping mode.
 - `stop` will still stop the trace and de-configure the hardware.
 - `dump` will dump the trace to a file.

### GDBSERVER
At the moment gdbserver is only supported on P8 while the cores are in the
kernel. 

To run a gdbserver on a P8 machine from a BMC running openbmc:

Stop all the threads of the core you want to look at
$ ./pdbg -d p8 -c11 -a stop

Run gdbserver on thread 0 of core 11, accessible through port 44
$ ./pdbg -d p8 -p0 -c11 -t0 gdbserver 44

On your local machine:
$ gdb
(gdb)  set architecture powerpc:common64
(gdb) target remote palm5-bmc:44

Debugging info:
(gdb) set debug remote 10


Notes:
1. DON'T RUN PDBG OVER FSI WHILE HOSTBOOT IS RUNNING. Weird things seem to
happen.
2. If you want to view the kernel call trace then run gdb on the vmlinux that
the host is running (the kernel needs to be compiled with debug symbols).
