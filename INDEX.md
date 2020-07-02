libpdbg Device Tree Model								{#mainpage}
=================================

[//]: # (Doxygen has an annoying "feature" meaning header id labels can only appear under others)

# Device Tree Model #											{#top}

Introduction
---------------

[dt_ref]: https://elinux.org/Device_Tree_Reference

The device tree format was originally designed to represent system
topology in small embedded systems and is used extensively both within
POWER for passing information to the Linux kernel and within ARM. For
an introduction see
[https://elinux.org/Device_Tree_Reference][dt_ref][1].

libpdbg implements a device tree model to track the state of the
entire system, what resources are available and to translate addresses
between local offsets to global bus addresses using the same format as
the Linux kernel as documented at
[https://elinux.org/Device_Tree_Usage]. At present the actual bindings
used are POWER specific and do not apply to the kernel.

A specific target is represented as a node in the device-tree with
appropriate `reg`, `range`, `#address-cells`, `#size-cells` as
described in [[1]][dt_ref] to describe any address translation. For
example:

	core@0 {
		#address-cells = <0x1>;
		#size-cells = <0x0>;
		compatible = "ibm,power9-core";
		index = <0x0>;
		reg = <0x0 0x0 0xfffff>;
		inside-special-wakeup = <0x0>;

		thread@0 {
				compatible = "ibm,power9-thread";
				reg = <0x0>;
				index = <0x0>;
		};
	};

\note Some device-tree addressing modes have yet to be implemented,
 see @ref target-type-specification for the current workaround.

Each device-node has a single parent and may have one or more
children. For example the above example has a single child. Each node
is also assigned a type/class via the compatible property. This
describes what each node represents in the system and allows discovery
of particular target types within a system. Nodes may also contain
arbitrary properties assoicated with each instance of that node.

The above representation of a node may also be saved to disk/memory as
a flattened device tree which may be processed using the standard
device tree compiler (dtc) available as a package for most Linux
distributions or in source form at
[https://github.com/dgibson/dtc]. The generation of flattened device
trees is also simple enough to embed in other projects.

libpdbg API overview									{#overview}
===========================

## Node representation ##

The [libpdbg API](@ref libpdbg.h) provides functions for building up a
complete device-tree representation of a POWER system based on the
current system state along with functions for traversing the tree.

Each node is represented in a program by a @ref pdbg_target. This is
used with most API functions to specify the target of a particular
command. At a minimum each node also has a parent node and a
class/type. There is no limit to the number of types of nodes
available. Each type is referenced with a name or by a handle.

## Tree traversal ##

Several macros are provided for finding desired nodes in a tree:

* Loop over every target of a specific type - @ref pdbg_for_each_class_target()
* Loop over direct children of a target - @ref pdbg_for_each_child_target()
* Loop over every target with a given parent/type - @ref pdbg_for_each_target()
* Get a targets parent - @ref pdbg_target_parent()

\note These functions are considered stable and won't change but
others will be added as required/requested.

## Property Manipulation ##

Device tree properties are stored as a series of bytes. Several
functions are available for getting and setting properties of various
types:

* Set a property value as a byte array - pdbg_target_set_property()
* Get a property as a byte array - pdbg_target_property()
* Get a property as a uint32 - pdbg_target_u32_property()
* Get a single uint32 from an array - pdbg_target_u32_index()

\note These are the only functions that have been required to date,
others will be added as required/requested.

## Hardware Support ##							{#hardware-support}

libpdbg provides abstractions for accessing hardware (eg. SCOM
registers) for multiple platforms and host processor types. Currently
supported platforms include:

* AMI based BMC systems via:
  + I2C (POWER8 only)
  + Userspace SoftFSI
* AST2500 OpenBMC based systems via:
  + SoftFSI (default)
  + Userspace SoftFSI
  + I2C (POWER8 only)
* (planned) AST2600 OpenBMC based systems via:
  + HardFSI (default)
  + Userspace SoftFSI
* Host OS:
  + XSCOM (default)

\todo SBEFIFO chip-op support is being developed and is roughly 80%
code complete. It will become the default for OpenBMC based systems
including eBMC.

Supported processor platforms include POWER8, POWER8NV, POWER9,
POWER9P/Axone (planned) and POWER10 (planned).

## Hardware Access ## {#hardware-access}

Functions for accessing hardware registers:

* Reading a CFAM register - fsi_read()
* Writing a CFAM register - fsi_write()
* Reading a SCOM - pib_read()
* Writing a SCOM - pib_write()
* Reading memory - mem_read()
* Writing memory - mem_write()

These function all take a specific pdbg_target. Therefore addressing
is local to the pdbg_target. Translation of relative addresses to
absolute bus addresses occurs either via normal device-tree addressing
rules or via target type specific callbacks as described in @ref
target-type-specification.

\todo The above list is not complete. See @ref libpdbg.h documentation
for a complete list of HW access functions.

## Target Type Specification ##			{#target-type-specification}

Targets types are specified as part of the library using fields with
in the pdbg_target struct. For example from libpdbg/xbus.c:

	struct xbus p9_xbus = {
		.target = {
			.name = "POWER9 XBus",
			.compatible = "ibm,xbus",
			.class = "xbus",
			.probe = xbus_probe,
			.translate = translate_cast(xbus_translate),
		},
	};
	DECLARE_HW_UNIT(p9_xbus);

This is used to match device nodes to a particular class/type and
allows customised functions for detecting presence and translating
target local addresses.

\note Not all device nodes require an associated HW unit, however
those without will not have any type and hence can only be accessed
via parent/child tree traversal functions. Normal device-tree
translation rules still apply.

\todo The above interfaces are private to libpdbg. These will be
exported to allow application specific types to be specified.

Frontends
============

For programmatic access it is preferable to use the [libpdbg API](@ref
libpdbg.h) directly. For command line usage there are two primary
frontends available. Both frontends will work on the platforms
specified in @ref hardware-access.

### pdbg ###

This frontend is included with libpdbg in the src/ directory. Complete
documentation is still a work-in-progress but some is available in the
[Pdbg readme](@ref README.md) and in the program help accessed via
`pdbg --help`

### eCMD ###

There is a eCMD based frontend available at
https://github.com/open-power/ecmd-pdbg. This provides a command-line
interface matching what is used in the lab (eg. `getscom pu f000f`).

\note The eCMD plugin is still under active development and not all
features are fully supported.

Language Bindings / Foreign Function Interfaces (FFI)
=======================================================================

C++
----

### Directly ###

C++ was designed to be compatible with C headers and
libraries. Therefore pdbg library functions can be called directly
from C++ code by including libpdbg.h

### FAPI2 ###

There is a binding available based on the FAPI2 specification. This
allows procedures written against FAPI2 to be compiled against the
device-tree based model without any changes to the existing
procedures. Procedures will therefore function on all of the platforms
listed under @ref hardware-support.

However cares needs to be taken when constructing the device-tree to
ensure the hiearchy matches what these procedures expect and that
target local address offsets get translated in the same manner.

Others
-------

Bindings for other languages should be straight forward to implement
however none are planned at this stage.

\note It is possible the eCMD plugin may provide a method of binding
to other languages
