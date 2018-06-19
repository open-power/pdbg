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
    BMC GPIO bit-banging FSI master
        CFAM hMFSI Port
            p1: POWER FSI2PIB
                POWER9 ADU
                c16: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c17: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c18: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c19: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c20: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c21: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c22: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
                c23: POWER9 Core
                    t0: POWER9 Thread
                    t1: POWER9 Thread
                    t2: POWER9 Thread
                    t3: POWER9 Thread
        p0: POWER FSI2PIB
            POWER9 ADU
            c5: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c7: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c14: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c15: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c19: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c20: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c21: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread
            c22: POWER9 Core
                t0: POWER9 Thread
                t1: POWER9 Thread
                t2: POWER9 Thread
                t3: POWER9 Thread

Note that only selected targets will be shown above. If none are shown
try adding '-a' to select all targets
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
$ ./pdbg -a getscom 0xf000f
p0:0xf000f = 0x220ea04980000000
p1:0xf000f = 0x220ea04980800000
```

### Write SCOM register on secondary processor
`$ ./pdbg -p1 putscom 0x8013c02 0x0`

### Get thread status
```
$ ./pdbg -a threadstatus

p0t: 0 1 2 3 4 5 6 7
c22: A A A A
c21: A A A A
c20: A A A A
c19: A A A A
c15: A A A A
c14: A A A A
c07: A A A A
c05: A A A A 

p1t: 0 1 2 3 4 5 6 7
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

p0t: 0 1 2 3 4 5 6 7
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

p0t: 0 1 2 3 4 5 6 7
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
00000000  68 65 6c 6c 6f 0a                                 |hello.|
00000006
```

### Write to cache-inhibited memory through processor 1
```
$ echo hello | sudo ./pdbg -p 1 putmem -ci 0x3fe88202
Wrote 6 bytes starting at 0x000000003fe88202
```

### Read from cache-inhibited memory through processor 1
```
$ sudo ./pdbg -p 1 getmem -ci 0x3fe88202 6 | hexdump -C
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

$ sudo ./pdbg -p 0  getmem -ci 0x0003ffff40000000 4 |hexdump -C
00000000  01 c0 d1 79                                       |...y|
00000004
$  sudo ./pdbg -p 0  getmem -ci 0x0003ffff40000000 4 |hexdump -C
00000000  77 9b ab ce                                       |w...|
00000004
$  sudo ./pdbg -p 0  getmem -ci 0x0003ffff40000000 4 |hexdump -C
00000000  66 8d fb 42                                       |f..B|
00000004
$  sudo ./pdbg -p 0  getmem -ci 0x0003ffff40000000 4 |hexdump -C
00000000  fa 9b e3 44                                       |...D|
00000004
```

### Hardware Trace Macro

Expoitation of HTM is limited to POWER9 NestHTM from the powerpc host.
POWER8 (core and nest).

pdbg provides a `htm` command with a variety of subcommands the most
useful command is `record` which will start the trace, wait for buffer
to fill (~1 sec), stop and then dump (~5 sec). eg.
```
pdbg -p0 -c4 core htm record
```
There are also low level commands which can also be used:
 - `start` will configure the hardware and start tracing in wrapping mode.
 - `stop` will still stop the trace and deconfigure the hardware
 - `dump` will dump the trace to a file

If you are running into a checkstop issue, `htm status` will print the
physical address of the buffer it is tracing into and the BMC can be
used to recover this memory after checkstop see `getmem`.

Using HTM requires a kernel built with both `CONFIG_PPC_MEMTRACE=y`
(v4.14) and `CONFIG_SCOM_DEBUGFS=y`. debugfs should be mounted at
`/sys/kernel/debug`. Ubuntu 18.04 has this by default.
