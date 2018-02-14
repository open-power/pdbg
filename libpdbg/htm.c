/* Copyright 2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "operations.h"
#include "bitutils.h"
#include "target.h"

#define HTM_ERR(x) ({int rc = x; if (rc) {PR_ERROR("HTM Error %d %s:%d\n", \
			rc, __FILE__, __LINE__);} \
			rc;})
/*
 * #define PR_PERL(x, args...) \
 *	fprintf(stderr, x, ##args)
 */
#define PR_PERL(...)

#define MIN(x,y) ((x < y) ? x : y)

#define DEBUGFS_POWERPC "/sys/kernel/debug/powerpc"
#define DEBUGFS_SCOM DEBUGFS_POWERPC"/scom"
#define DEBUGFS_MEMTRACE DEBUGFS_POWERPC"/memtrace"
#define DEBUGFS_MEMTRACE_ENABLE DEBUGFS_MEMTRACE"/enable"

#define HTM_COLLECTION_MODE		0
#define	  HTM_MODE_ENABLE		PPC_BIT(0)
#define	  HTM_MODE_CONTENT_SEL		PPC_BITMASK(1,2)
#define   NHTM_MODE_CAPTURE		PPC_BITMASK(4,12)
#define     NHTM_MODE_CAPTURE_PMISC	PPC_BIT(5)
#define     NHTM_MODE_CRESP_PRECISE	PPC_BIT(6)
#define     CHTM_MODE_NO_ASSERT_LLAT_L3 PPC_BIT(6)
#define	  HTM_MODE_WRAP			PPC_BIT(13)
#define HTM_MEMORY_CONF			1
#define   HTM_MEM_ALLOC			PPC_BIT(0)
#define   HTM_MEM_SCOPE			PPC_BITMASK(1,3)
#define   HTM_MEM_PRIORITY		PPC_BIT(4)
#define   HTM_MEM_SIZE_SMALL		PPC_BIT(5)
#define   HTM_MEM_BASE			PPC_BITMASK(8,39)
#define	  HTM_MEM_SIZE			PPC_BITMASK(40,48)
#define HTM_STATUS			2
#define   HTM_STATUS_MASK		PPC_BITMASK(2,19)
#define   HTM_STATUS_CRESP_OV		PPC_BIT(2)
#define	  HTM_STATUS_REPAIR		PPC_BIT(3)
#define   HTM_STATUS_BUF_WAIT		PPC_BIT(4)
#define   HTM_STATUS_TRIG_DROPPED	PPC_BIT(5)
#define   HTM_STATUS_ADDR_ERROR		PPC_BIT(6)
#define   HTM_STATUS_REC_DROPPED	PPC_BIT(7)
#define   HTM_STATUS_INIT		PPC_BIT(8)
#define   HTM_STATUS_PREREQ		PPC_BIT(9)
#define   HTM_STATUS_READY		PPC_BIT(10)
#define   HTM_STATUS_TRACING		PPC_BIT(11)
#define   HTM_STATUS_PAUSED		PPC_BIT(12)
#define   HTM_STATUS_FLUSH		PPC_BIT(13)
#define   HTM_STATUS_COMPLETE		PPC_BIT(14)
#define   HTM_STATUS_ENABLE		PPC_BIT(15)
#define   HTM_STATUS_STAMP		PPC_BIT(16)
#define   HTM_STATUS_SCOM_ERROR		PPC_BIT(17)
#define   HTM_STATUS_PARITY_ERROR	PPC_BIT(18)
#define   HTM_STATUS_INVALID_CRESP	PPC_BIT(19)
#define   HTM_STATUS_STATE_MASK		(PPC_BITMASK(8,16) | HTM_STATUS_REPAIR)
#define	  HTM_STATUS_ERROR_MASK		(HTM_STATUS_CRESP_OV | PPC_BITMASK(4,7) | PPC_BITMASK(17,19))
#define HTM_LAST_ADDRESS		3
#define   HTM_LAST_ADDRESS_MASK		PPC_BITMASK(8,56)
#define HTM_SCOM_TRIGGER		4
#define	  HTM_TRIG_START		PPC_BIT(0)
#define   HTM_TRIG_STOP			PPC_BIT(1)
#define   HTM_TRIG_PAUSE		PPC_BIT(2)
#define   HTM_TRIG_STOP_ALT		PPC_BIT(3)
#define   HTM_TRIG_RESET		PPC_BIT(4)
#define   HTM_TRIG_MARK_VALID		PPC_BIT(5)
#define   HTM_TRIG_MARK_TYPE		PPC_BITMASK(6,15)
#define HTM_TRIGGER_CONTROL		5
#define   HTM_CTRL_TRIG			PPC_BITMASK(0,1)
#define   HTM_CTRL_MASK			PPC_BITMASK(4,5)
#define   HTM_CTRL_XSTOP_STOP		PPC_BIT(13)
#define NHTM_FILTER_CONTROL		6
#define   NHTM_FILTER_RCMD_SCOPE	PPC_BITMASK(17,19)
#define   NHTM_FILTER_RCMD_SOURCE	PPC_BITMASK(20,21)
#define   NHTM_FILTER_CRESP_PATTERN	PPC_BITMASK(27,31)
#define   NHTM_FILTER_MASK		PPC_BITMASK(32,54)
#define   NHTM_FILTER_CRESP_MASK	PPC_BITMASK(59,63)
#define NHTM_TTYPE_FILTER_CONTROL	7
#define   NHTM_TTYPE_TYPE_MASK		PPC_BITMASK(17,23)
#define   NHTM_TTYPE_SIZE_MASK		PPC_BITMASK(24,31)
#define NHTM_CONFIGURATION		8
#define   NHTM_CONF_HANG_DIV_RATIO	PPC_BITMASK(0,4)
#define   NHTM_CONF_RTY_DROP_COUNT	PPC_BITMASK(5,8)
#define   NHTM_CONF_DROP_PRIORITY_INC	PPC_BIT(9)
#define   NHTM_CONF_RETRY_BACKOFF	PPC_BIT(10)
#define   NHTM_CONF_OPERALIONAL_HANG	PPC_BIT(11)
#define NHTM_FLEX_MUX			9
#define   NHTM_FLEX_MUX_MASK		PPC_BITMASK(0,35)
#define   NHTM_FLEX_DEFAULT		0xCB3456129

enum htm_state {
	INIT,
	REPAIR,
	PREREQ,
	READY,
	TRACING,
	PAUSED,
	FLUSH,
	COMPLETE,
	ENABLE,
	STAMP,
	UNINITIALIZED,
};

enum htm_error {
	NONE,
	CRESP_OV,
	BUF_WAIT,
	TRIGGER_DROPPED,
	ADDR_ERROR,
	RECORD_DROPPED,
	SCOM_ERROR,
	PARITY_ERROR,
	INVALID_CRESP,
};

enum htm_size {
	MEM_16MB,
	MEM_32MB,
	MEM_64MB,
	MEM_128MB,
	MEM_256MB,
	MEM_512MB,
	MEM_1GB,
	MEM_2GB,
	MEM_4GB,
	MEM_8GB,
	MEM_16GB,
	MEM_32GB,
	MEM_64GB,
	MEM_128GB,
	MEM_256GB,
};

struct htm_status {
	enum htm_state state;
	enum htm_error error;
	bool mem_size_select;
	uint16_t mem_size;
	uint64_t mem_base;
	uint64_t mem_last;
	uint64_t raw;
};

static struct htm *check_and_convert(struct pdbg_target *target)
{

	if (!pdbg_target_is_class(target, "nhtm") &&
	    !pdbg_target_is_class(target, "chtm"))
	    return NULL;

	return target_to_htm(target);
}

int htm_start(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	return htm ? htm->start(htm) : -1;
}

int htm_stop(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	return htm ? htm->stop(htm) : -1;
}

int htm_reset(struct pdbg_target *target, uint64_t *base, uint64_t *size)
{
	struct htm *htm = check_and_convert(target);

	return htm ? htm->reset(htm, base, size) : -1;
}

int htm_pause(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	return htm ? htm->pause(htm) : -1;
}

int htm_status(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	return htm ? htm->status(htm) : -1;
}

int htm_dump(struct pdbg_target *target, uint64_t size, const char *filename)
{
	struct htm *htm = check_and_convert(target);

	if (!htm || !filename)
		return -1;

	return htm->dump(htm, 0, filename);
}

static int get_status(struct htm *htm, struct htm_status *status)
{
	uint64_t val;

	if (HTM_ERR(pib_read(&htm->target, HTM_STATUS, &status->raw)))
		return -1;

	switch (status->raw & HTM_STATUS_ERROR_MASK) {
	case HTM_STATUS_CRESP_OV:
		status->error = CRESP_OV;
		break;
	case HTM_STATUS_BUF_WAIT:
		status->error = BUF_WAIT;
		break;
	case HTM_STATUS_TRIG_DROPPED:
		status->error = TRIGGER_DROPPED;
		break;
	case HTM_STATUS_ADDR_ERROR:
		status->error = ADDR_ERROR;
		break;
	case HTM_STATUS_REC_DROPPED:
		status->error = RECORD_DROPPED;
		break;
	case HTM_STATUS_SCOM_ERROR:
		status->error = SCOM_ERROR;
		break;
	case HTM_STATUS_PARITY_ERROR:
		status->error = PARITY_ERROR;
		break;
	case HTM_STATUS_INVALID_CRESP:
		status->error = INVALID_CRESP;
		break;
	default:
		status->error = NONE;
	}

	switch (status->raw & HTM_STATUS_STATE_MASK) {
	case HTM_STATUS_REPAIR:
		status->state = REPAIR;
		break;
	case HTM_STATUS_INIT:
		status->state = INIT;
		break;
	case HTM_STATUS_PREREQ:
		status->state = PREREQ;
		break;
	case HTM_STATUS_READY:
		status->state = READY;
		break;
	case HTM_STATUS_TRACING:
		status->state = TRACING;
		break;
	case HTM_STATUS_PAUSED:
		status->state = PAUSED;
		break;
	case HTM_STATUS_FLUSH:
		status->state = FLUSH;
		break;
	case HTM_STATUS_COMPLETE:
		status->state = COMPLETE;
		break;
	case HTM_STATUS_ENABLE:
		status->state = ENABLE;
		break;
	case HTM_STATUS_STAMP:
		status->state = STAMP;
		break;
	default:
		status->state = UNINITIALIZED;
	}

	if (HTM_ERR(pib_read(&htm->target, HTM_MEMORY_CONF, &val)))
		return -1;

	status->mem_size_select = val & HTM_MEM_SIZE_SMALL;
	status->mem_size = GETFIELD(HTM_MEM_SIZE,val);
	status->mem_base = val & HTM_MEM_BASE;

	if (HTM_ERR(pib_read(&htm->target, HTM_LAST_ADDRESS, &status->mem_last)))
		return -1;

	return 0;
}

static int do_htm_stop(struct htm *htm)
{
	struct htm_status status;
	get_status(htm, &status);
	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (status.state == UNINITIALIZED) {
		PR_PERL("* Skipping STOP trigger, HTM appears uninitialized\n");
		return -1;
	}
	if (status.state == TRACING) {
		PR_PERL("* Sending STOP trigger to HTM\n");
		if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_STOP)))
			return -1;
	} else {
		PR_PERL("* Skipping STOP trigger, HTM is not running\n");
	}
	return 1;
}

/*
 * This sequence will trigger all HTM to start at "roughly" the same
 * time.
 * We don't have any documentation on what the commands are, they do
 * work on POWER9 dd2.0
 *
 * Alternatively, writing a 1 into the HTM Start Trigger bit works
 * too, it has to be done for each HTM but its much cleaner and
 * clearly what is going on.
 */
#if 0
static int do_adu_magic(struct pdbg_target *target, uint32_t index, uint64_t *arg1, uint64_t *arg2)
{
	uint64_t val;
	int i = 0;

	/*
	 * pib_write(target, 1, PPC_BIT(11); get the lock, but since
	 * we don't check, why bother?
	 */
	if (HTM_ERR(pib_write(target, 1, PPC_BIT(3) | PPC_BIT(4) | PPC_BIT(14))))
		return -1;

	/* If bothering with the lock, write PPC_BIT(11) here */
	if (HTM_ERR(pib_write(target, 1, 0)))
		return -1;

	if (HTM_ERR(pib_write(target, 0, PPC_BIT(46))))
		return -1;

	if (HTM_ERR(pib_write(target, 1, 0x2210aab140000000)))
		return -1;

	do {
		sleep(1);
		if (HTM_ERR(pib_read(target, 3, &val)))
			return -1;
		i++;
	} while (val != 0x2000000000000004 && i < 10);

	if (val != 0x2000000000000004) {
		PR_PERL("Unexpected status on HTM start trigger PMISC command: 0x%"
				PRIx64 "\n", val);
		return -1;
	}

	/*
	 * If we locked the adu before we should be polite and
	 * pib_write(target, 1, 0);
	 */

	return 1;
}
#endif

/*
 * This function doesn't do the update_mcs_regs() from the older p8
 * only tool. Not clear what this actually does. Attempting to ignore
 * the problem.
 * The scoms it does use are:
 * SCOM ADDRESS FOR MCS4
 * #define MCS4_MCFGPQ 0x02011c00
 * #define MCS4_MCMODE 0x02011c07
 * #define MCS4_FIRMASK 0x02011c43
 *
 * SCOM ADDRESS FOR MCS5
 * #define MCS5_MCFGPQ 0x02011c80
 * #define MCS5_MCMODE 0x02011c87
 * #define MCS5_FIRMASK 0x02011cc3
 *
 * SCOM ADDRESS FOR MCS6
 * #define MCS6_MCFGPQ 0x02011d00
 * #define MCS6_MCMODE 0x02011d07
 * #define MCS6_FIRMASK 0x02011d43
 *
 * SCOM ADDRESS FOR MCS7
 * #define MCS7_MCFGPQ 0x02011d80
 * #define MCS7_MCMODE 0x02011d87
 * #define MCS7_FIRMASK 0x02011dc3
 */

static int configure_debugfs_memtrace(struct htm *htm)
{
	uint64_t memtrace_size;
	FILE *file;

	file = fopen(DEBUGFS_MEMTRACE_ENABLE, "r+");
	if (!file) {
		PR_ERROR("Couldn't open %s: %m\n", DEBUGFS_MEMTRACE_ENABLE);
		return -1;
	}

	if (fscanf(file, "0x%016" PRIx64 "\n", &memtrace_size) != 1) {
		PR_ERROR("fscanf() didn't match: %m\n");
		fclose(file);
		return -1;
	}

	if (memtrace_size == 0) {
		uint64_t size = 1 << 30; /* 1GB... TODO */
		int rc = fprintf(file, "0x%016" PRIx64 "\n", size);
		PR_DEBUG("memtrace_size is zero! Informing the kernel!\n");
		if (rc <= 0) {
			PR_ERROR("Couldn't set memtrace_size\n");
			PR_ERROR("Attempted to write: 0x%016" PRIx64 " got %d: %m\n", size, rc);
			fclose(file);
			return -1;
		}

	}
	fclose(file);

	return 0;
}

static int configure_chtm(struct htm *htm)
{
	if (HTM_ERR(configure_debugfs_memtrace(htm)))
		return -1;

	if (HTM_ERR(pib_write(&htm->target, HTM_COLLECTION_MODE,
					HTM_MODE_ENABLE |
					CHTM_MODE_NO_ASSERT_LLAT_L3)))
		return -1;

	return 0;
}

static int configure_nhtm(struct htm *htm)
{
	uint64_t val;

	if (HTM_ERR(configure_debugfs_memtrace(htm)))
		return -1;

	/*
	 * The constant is the VGTARGET field, taken from a cronus
	 * booted system which presumably set it up correctly
	 */
	if (HTM_ERR(pib_write(&htm->target, HTM_COLLECTION_MODE,
					HTM_MODE_ENABLE |
					NHTM_MODE_CRESP_PRECISE |
					HTM_MODE_WRAP |
					0xFFFF000000)))
		return -1;

	/* Stop on core xstop */
	if (HTM_ERR(pib_write(&htm->target, HTM_TRIGGER_CONTROL, HTM_CTRL_XSTOP_STOP)))
		return -1;;

	/*
	 * These values are taken from a cronus booted system.
	 */
	if (HTM_ERR(pib_write(&htm->target, NHTM_FILTER_CONTROL,
			NHTM_FILTER_MASK | /* no pattern matching */
			NHTM_FILTER_CRESP_MASK))) /* no pattern matching */
		return -1;

	if (HTM_ERR(pib_write(&htm->target, NHTM_TTYPE_FILTER_CONTROL,
			NHTM_TTYPE_TYPE_MASK | /* no pattern matching */
			NHTM_TTYPE_SIZE_MASK ))) /* no pattern matching */
		return -1;

	if (dt_node_is_compatible(htm->target.dn, "ibm,power9-nhtm")) {
		if (HTM_ERR(pib_read(&htm->target, NHTM_FLEX_MUX, &val)))
			return -1;

		if (GETFIELD(NHTM_FLEX_MUX_MASK, val) != NHTM_FLEX_DEFAULT) {
			PR_ERROR("The HTM Flex wasn't default value\n");
			return -1;
		}
	}

	return 0;
}

static int is_startable(struct htm_status *status)
{
	return (status->state == READY || status->state == PAUSED);
}

static int do_htm_start(struct htm *htm)
{
	struct htm_status status;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (!is_startable(&status)) {
		PR_ERROR("HTM not in a startable state!\n");
		return -1;
	}

	PR_PERL("* Sending START trigger to HTM\n");
	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_MARK_VALID)))
		return -1;

	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_START)))
		return -1;

	/*
	 * Instead of the HTM_TRIG_START, this is where you might want
	 * to call do_adu_magic()
	 * for_each_child_target("adu", htm->target.dn->parent->target, do_adu_magic, NULL, NULL);
	 * see what I mean?
	 */

	return 1;
}

static char *get_debugfs_file(struct htm *htm, const char *file)
{
	uint32_t chip_id;
	char *filename;

	chip_id = dt_get_chip_id(htm->target.dn);
	if (chip_id == -1) {
		PR_ERROR("Couldn't find a chip id\n");
		return NULL;
	}

	if (asprintf(&filename, "%s/%08x/%s", DEBUGFS_MEMTRACE, chip_id, file) == -1) {
		PR_ERROR("Couldn't asprintf() '%s/%08x/size': %m\n",
				DEBUGFS_MEMTRACE, chip_id);
		return NULL;
	}

	return filename;
}

static int get_trace_size(struct htm *htm, uint64_t *size)
{
	char *filename;
	FILE *file;
	int rc;

	filename = get_debugfs_file(htm, "size");
	if (!filename)
		return -1;

	file = fopen(filename, "r");
	if (file == NULL) {
		PR_ERROR("Failed to open %s: %m\n", filename);
		free(filename);
		return -1;
	}

	rc = fscanf(file, "0x%016" PRIx64 "\n", size);
	fclose(file);
	free(filename);

	return !(rc == 1);
}

static int get_trace_base(struct htm *htm, uint64_t *base)
{
	char *filename;
	FILE *file;
	int rc;

	filename = get_debugfs_file(htm, "start");
	if (!filename)
		return -1;

	file = fopen(filename, "r");
	if (file == NULL) {
		PR_ERROR("Failed to open %s: %m\n", filename);
		free(filename);
		return -1;
	}


	rc = fscanf(file, "0x%016" PRIx64 "\n", base);
	fclose(file);
	free(filename);

	return !(rc == 1);
}

static bool is_resetable(struct htm_status *status)
{
	return status->state == COMPLETE || status->state == REPAIR || status->state == INIT;
}

static bool is_configured(struct htm *htm)
{
	struct htm_status status;
	uint64_t val;

	if (HTM_ERR(get_status(htm, &status)))
		return false;

	/*
	 * We've most likely just booted and everything is zeroed
	 */
	if (status.raw == 0 && status.mem_base == 0 && status.mem_size == 0)
		return false;

	if (HTM_ERR(pib_read(&htm->target, HTM_COLLECTION_MODE, &val)))
		return false;

	if (!(val & HTM_MODE_ENABLE))
		return false;

	return true;
}
static int configure_memory(struct htm *htm, uint64_t *r_base, uint64_t *r_size)
{
	uint64_t i, size, base, val;
	uint16_t mem_size;

	if (HTM_ERR(get_trace_size(htm, &size)))
		return -1;

	if (HTM_ERR(get_trace_base(htm, &base)))
		return -1;

	if (HTM_ERR(pib_read(&htm->target, HTM_MEMORY_CONF, &val)))
		return -1;

	/*
	 * Tell HTM that we've alloced mem, perhaps do this in probe(),
	 * HTM searches for 0 -> 1 transition, so clear it now
	 */
	val &= ~HTM_MEM_ALLOC;
	if (HTM_ERR(pib_write(&htm->target, HTM_MEMORY_CONF, val)))
		return -1;

	if (HTM_ERR(pib_read(&htm->target, HTM_MEMORY_CONF, &val)))
		return -1;

	/* Put mem alloc back in */
	val |= HTM_MEM_ALLOC;

	mem_size = 0;
	i = size;
	while ((!((val & HTM_MEM_SIZE_SMALL) && i>>20 == 16)) && (!(!(val & HTM_MEM_SIZE_SMALL) && i>>20 == 512))) {
		mem_size <<= 1;
		mem_size++;
		i >>= 1;
	}

	val = SETFIELD(HTM_MEM_SIZE, val, mem_size);
	/*
	 * Clear out the base
	 * Don't use SETFIELD to write the base in since the value is
	 * already shifted correct as read from the kernel, or'ing it
	 * in works fine.
	 */
	val = SETFIELD(HTM_MEM_BASE, val, 0);
	val |= base;

	if (HTM_ERR(pib_write(&htm->target, HTM_MEMORY_CONF, val)))
		return -1;

	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_RESET)))
		return -1;

	if (r_size)
		*r_size = size;
	if (r_base)
		*r_base = base;

	return 0;
}

static int do_htm_reset(struct htm *htm, uint64_t *r_base, uint64_t *r_size)
{
	struct htm_status status;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (!is_resetable(&status) || !is_configured(htm)) {
		if (pdbg_target_is_class(&htm->target, "nhtm")) {
			if (HTM_ERR(configure_nhtm(htm))) {
				PR_ERROR("Couldn't setup Nest HTM during reset\n");
				return -1;
			}
		} else if (pdbg_target_is_class(&htm->target, "chtm")) {
			if (HTM_ERR(configure_chtm(htm))) {
				PR_ERROR("Couldn't setup Core HTM during reset\n");
				return -1;
			}
		} else { /* Well... */
			PR_ERROR("Unknown HTM class! %s\n",
				pdbg_target_class_name(&htm->target));
		}

	}

	if (HTM_ERR(configure_memory(htm, r_base, r_size)))
		return -1;

	return 1;
}

static int do_htm_pause(struct htm *htm)
{
	struct htm_status status;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (status.state == UNINITIALIZED) {
		PR_PERL("* Skipping PAUSE trigger, HTM appears uninitialized\n");
		return 0;
	}

	PR_PERL("* Sending PAUSE trigger to HTM\n");
	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_PAUSE)))
		return -1;

	return 0;
}

static int do_htm_status(struct htm *htm)
{
	struct htm_status status;
	uint64_t val, total;
	int i, regs = 9;

	if (dt_node_is_compatible(htm->target.dn, "ibm,power9-nhtm"))
		regs++;

	PR_DEBUG("HTM register dump:\n");
	for (i = 0; i < regs; i++) {
		if (HTM_ERR(pib_read(&htm->target, i, &val))) {
			PR_ERROR("Couldn't read HTM reg: %d\n", i);
			continue;
		}

		PR_DEBUG(" %d: 0x%016" PRIx64 "\n", i, val);
	}
	putchar('\n');

	PR_INFO("* Checking HTM status...\n");
	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (status.mem_size_select)
		total = 16;
	else
		total = 512;

	while (status.mem_size) {
		total <<= 1;
		status.mem_size >>= 1;
	}

	PR_DEBUG("HTM status : 0x%016" PRIx64 "\n", status.raw);
	PR_INFO("State: ");
	switch (status.state) {
	case INIT:
		PR_INFO("INIT");
		break;
	case PREREQ:
		PR_INFO("PREREQ");
		break;
	case READY:
		PR_INFO("READY");
		break;
	case TRACING:
		PR_INFO("TRACING");
		break;
	case PAUSED:
		PR_INFO("PAUSED");
		break;
	case FLUSH:
		PR_INFO("FLUSH");
		break;
	case COMPLETE:
		PR_INFO("COMPLETE");
		break;
	case ENABLE:
		PR_INFO("ENABLE");
		break;
	case STAMP:
		PR_INFO("STAMP");
		break;
	default:
		PR_INFO("UNINITIALIZED");
	}
	PR_INFO("\n");

	PR_INFO("HTM base addr : 0x%016" PRIx64 ", size: 0x%016" PRIx64 " ",
			status.mem_base, total << 20);
	if (total > 512)
		PR_INFO("[ %" PRIu64 "GB ]", total >> 10);
	else
		PR_INFO("[ %" PRIu64 "MB ]", total);
	PR_INFO("\n");
	return 1;
}

static int do_htm_dump(struct htm *htm, uint64_t size, const char *basename)
{
	char *trace_file, *dump_file;
	struct htm_status status;
	uint64_t trace[0x1000];
	int trace_fd, dump_fd;
	uint32_t chip_id;
	size_t r;

	if (!basename)
		return -1;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (status.state != COMPLETE) {
		PR_PERL("* Skipping DUMP tigger, HTM is not in complete state\n");
		return -1;
	}

	chip_id = dt_get_chip_id(htm->target.dn);
	if (chip_id == -1)
		return -1;

	/* If size is zero they must want the entire thing */
	if (size == 0)
		size = status.mem_last - status.mem_base;

	if (status.mem_last - status.mem_base > size) {
		PR_INFO("Requested size is larger than trace 0x%" PRIx64" vs 0x%" PRIx64 "\n",
				size, status.mem_last - status.mem_base);
		size = status.mem_last - status.mem_base;
		PR_INFO("Truncating request to a maxiumum of 0x%" PRIx64 "bytes\n", size);
	}

	trace_file = get_debugfs_file(htm, "trace");
	if (!trace_file)
		return -1;

	trace_fd = open(trace_file, O_RDWR);
	if (trace_fd == -1) {
		PR_ERROR("Failed to open %s: %m\n", trace_file);
		free(trace_file);
		return -1;
	}

	if (asprintf(&dump_file, "%d.%d-%s", chip_id, htm->target.index, basename) == -1) {
		free(trace_file);
		close(trace_fd);
		return -1;
	}
	dump_fd = open(dump_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (dump_fd == -1) {
		PR_ERROR("Failed to open %s: %m\n", dump_file);
		free(dump_file);
		free(trace_file);
		close(trace_fd);
		return -1;
	}

	while (size) {
		r = read(trace_fd, &trace, MIN(sizeof(trace), size));
		if (r == -1) {
			PR_ERROR("Failed to read from %s: %m\n", trace_file);
			free(trace_file);
			free(dump_file);
			close(trace_fd);
			close(dump_fd);
			return -1;
		}

		if (write(dump_fd, &trace, r) != r)
			PR_ERROR("Short write!\n");

		size -= r;
	}

	free(trace_file);
	free(dump_file);
	close(trace_fd);
	close(dump_fd);
	return 1;
}

static bool is_debugfs_memtrace_ok(void)
{
	return access(DEBUGFS_MEMTRACE, F_OK) == 0;
}

static bool is_debugfs_scom_ok(void)
{
	return access(DEBUGFS_SCOM, F_OK) == 0;
}

static int nhtm_probe(struct pdbg_target *target)
{
	uint64_t val;

	if (!is_debugfs_memtrace_ok() || !is_debugfs_scom_ok())
		return -1;

	if (dt_node_is_compatible(target->dn, "ibm,power9-nhtm")) {
		pib_read(target, NHTM_FLEX_MUX, &val);
		if (GETFIELD(NHTM_FLEX_MUX_MASK, val) != NHTM_FLEX_DEFAULT) {
			PR_DEBUG("FLEX_MUX not default\n");
			return -1;
		}
	}

	return 0;
}

static int chtm_probe(struct pdbg_target *target)
{
	return is_debugfs_memtrace_ok() && is_debugfs_scom_ok() ? 0 : -1;
}

struct htm nhtm = {
	.target = {
		.name =	"Nest HTM",
		.compatible = "ibm,power8-nhtm", "ibm,power9-nhtm",
		.class = "nhtm",
		.probe = nhtm_probe,
	},
	.start = do_htm_start,
	.stop = do_htm_stop,
	.reset = do_htm_reset,
	.pause = do_htm_pause,
	.status = do_htm_status,
	.dump = do_htm_dump,
};
DECLARE_HW_UNIT(nhtm);

struct htm chtm = {
	.target = {
		.name = "POWER8 Core HTM",
		.compatible = "ibm,power8-chtm",
		.class = "chtm",
		.probe = chtm_probe,
	},
	.start = do_htm_start,
	.stop = do_htm_stop,
	.reset = do_htm_reset,
	.pause = do_htm_pause,
	.status = do_htm_status,
	.dump = do_htm_dump,
};
DECLARE_HW_UNIT(chtm);
