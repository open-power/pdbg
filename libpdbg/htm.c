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
#include "hwunit.h"
#include "debug.h"

#define HTM_ERR(x) ({int rc = (x); if (rc) {PR_ERROR("HTM Error %d %s:%d\n", \
			rc, __FILE__, __LINE__);} \
			rc;})

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define DEBUGFS_POWERPC "/sys/kernel/debug/powerpc"
#define DEBUGFS_SCOM DEBUGFS_POWERPC"/scom"
#define DEBUGFS_MEMTRACE DEBUGFS_POWERPC"/memtrace"
#define DEBUGFS_MEMTRACE_ENABLE DEBUGFS_MEMTRACE"/enable"

/*
 * This is a CORE register not a HTM register, don't pass the HTM
 * target to it.
 */
#define HID0_REGISTER			0x1329C
#define  HID0_ONE_PER_GROUP		PPC_BIT(0)
#define  HID0_DO_SINGLE			PPC_BIT(1)
#define  HID0_SINGLE_DECODE		PPC_BIT(4)
#define  HID0_EN_INST_TRACE		PPC_BIT(17)
#define  HID0_TRACE_EN			PPC_BIT(23)
#define  HID0_TRACE_BITS (HID0_ONE_PER_GROUP | HID0_DO_SINGLE |\
			  HID0_SINGLE_DECODE | HID0_EN_INST_TRACE |\
			  HID0_TRACE_EN)

/*
 * This is a CORE register not a HTM register, don't pass the HTM
 * target to it.
 */
#define NCU_MODE_REGISTER		0x10C0A
#define NCU_MODE_HTM_ENABLE		PPC_BIT(0)

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
/* Note: the next 3 change on P9 */
#define   HTM_MEM_PRIORITY		PPC_BITMASK(4,5)
#define   HTM_MEM_PRIORITY_P10		PPC_BITMASK(4)
#define   HTM_MEM_SIZE_SMALL		PPC_BIT(13)
#define   HTM_MEM_SIZE_SMALL_P10	PPC_BIT(5)
#define   HTM_MEM_BASE			PPC_BITMASK(14,39)
#define   HTM_MEM_BASE_P10		PPC_BITMASK(8,39)
#define	  HTM_MEM_SIZE			PPC_BITMASK(40,48)
#define HTM_STATUS			2
#define   HTM2_STATUS_MASK		PPC_BITMASK(22,39)
#define   HTM2_STATUS_CRESP_OV		PPC_BIT(22)
#define	  HTM2_STATUS_REPAIR		PPC_BIT(23)
#define   HTM2_STATUS_BUF_WAIT		PPC_BIT(24)
#define   HTM2_STATUS_TRIG_DROPPED	PPC_BIT(25)
#define   HTM2_STATUS_ADDR_ERROR	PPC_BIT(26)
#define   HTM2_STATUS_REC_DROPPED	PPC_BIT(27)
#define   HTM2_STATUS_INIT		PPC_BIT(28)
#define   HTM2_STATUS_PREREQ		PPC_BIT(29)
#define   HTM2_STATUS_READY		PPC_BIT(30)
#define   HTM2_STATUS_TRACING		PPC_BIT(31)
#define   HTM2_STATUS_PAUSED		PPC_BIT(32)
#define   HTM2_STATUS_FLUSH		PPC_BIT(33)
#define   HTM2_STATUS_COMPLETE		PPC_BIT(34)
#define   HTM2_STATUS_ENABLE		PPC_BIT(35)
#define   HTM2_STATUS_STAMP		PPC_BIT(36)
#define   HTM2_STATUS_SCOM_ERROR	PPC_BIT(37)
#define   HTM2_STATUS_PARITY_ERROR	PPC_BIT(38)
#define   HTM2_STATUS_INVALID_CRESP	PPC_BIT(39)
#define   HTM2_STATUS_STATE_MASK	(PPC_BITMASK(28,36) | HTM_STATUS_REPAIR)
#define	  HTM2_STATUS_ERROR_MASK	(HTM_STATUS_CRESP_OV | PPC_BITMASK(24,27) | PPC_BITMASK(37,39))
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
#define HTM2_LAST_ADDRESS		4
#define   HTM2_LAST_ADDRESS_MASK	PPC_BITMASK(8,56)
#define HTM_SCOM_TRIGGER		(4 + dual_htm_offset)
#define	  HTM_TRIG_START		PPC_BIT(0)
#define   HTM_TRIG_STOP			PPC_BIT(1)
#define   HTM_TRIG_PAUSE		PPC_BIT(2)
#define   HTM_TRIG_STOP_ALT		PPC_BIT(3)
#define   HTM_TRIG_RESET		PPC_BIT(4)
#define   HTM_TRIG_MARK_VALID		PPC_BIT(5)
#define   HTM_TRIG_MARK_TYPE		PPC_BITMASK(6,15)
#define HTM_TRIGGER_CONTROL		(5 + dual_htm_offset)
#define   HTM_CTRL_TRIG			PPC_BITMASK(0,1)
#define   HTM_CTRL_MASK			PPC_BITMASK(4,5)
#define   HTM_CTRL_XSTOP_STOP		PPC_BIT(13)
#define NHTM_FILTER_CONTROL		(6 + dual_htm_offset)
#define   NHTM_FILTER_RCMD_SCOPE	PPC_BITMASK(17,19)
#define   NHTM_FILTER_RCMD_SOURCE	PPC_BITMASK(20,21)
#define   NHTM_FILTER_CRESP_PATTERN	PPC_BITMASK(27,31)
#define   NHTM_FILTER_MASK		PPC_BITMASK(32,54)
#define   NHTM_FILTER_CRESP_MASK	PPC_BITMASK(59,63)
#define NHTM_TTYPE_FILTER_CONTROL	(7 + dual_htm_offset)
#define   NHTM_TTYPE_TYPE_MASK		PPC_BITMASK(17,23)
#define   NHTM_TTYPE_SIZE_MASK		PPC_BITMASK(24,31)
#define NHTM_CONFIGURATION		(8 + dual_htm_offset)
#define   NHTM_CONF_HANG_DIV_RATIO	PPC_BITMASK(0,4)
#define   NHTM_CONF_RTY_DROP_COUNT	PPC_BITMASK(5,8)
#define   NHTM_CONF_DROP_PRIORITY_INC	PPC_BIT(9)
#define   NHTM_CONF_RETRY_BACKOFF	PPC_BIT(10)
#define   NHTM_CONF_OPERALIONAL_HANG	PPC_BIT(11)
#define NHTM_FLEX_MUX			(9 + dual_htm_offset)
#define   NHTM_FLEX_MUX_MASK		PPC_BITMASK(0,35)
#define   NHTM_FLEX_DEFAULT		0xCB3456129
#define   NHTM_FLEX_MUX_MASK_P10	PPC_BITMASK(0,39)
#define   NHTM_FLEX_DEFAULT_P10		0xCB34561567
#define NHTM_ADDR_PAT			0xb
#define   NHTM_ADDR_PAT_MASK		PPC_BITMASK(0, 55)
#define NHTM_ADDR_MASK			0xc
#define   NHTM_ADDR_MASK_MASK		PPC_BITMASK(0, 55)
#define NHTM_STOP_FILTER		0xd
#define   NHTM_STOP_FILTER_TTAG_PAT	PPC_BITMASK(0, 14)
#define   NHTM_STOP_FILTER_TTAG_MASK	PPC_BITMASK(15, 29)
#define   NHTM_STOP_FILTER_TTYPE_PAT	PPC_BITMASK(30, 36)
#define   NHTM_STOP_FILTER_TTYPE_MASK	PPC_BITMASK(37, 43)
#define   NHTM_STOP_FILTER_CRESP_PAT	PPC_BITMASK(44, 48)
#define   NHTM_STOP_FILTER_CRESP_MASK	PPC_BITMASK(49, 53)
#define   NHTM_STOP_CYCLES		PPC_BITMASK(54, 60)
#define NHTM_STOP_FILT_ADDR_PAT		0xe
#define   NHTM_STOP_FILT_ADDR_PAT_MASK	PPC_BITMASK(0, 55)
#define NHTM_STOP_FILT_ADDR_MASK	0xf
#define   NHTM_STOP_FILT_ADDR_MASK_MASK	PPC_BITMASK(0, 55)

/* On P10 there are two nhtms which shifts some of the registers */
static int dual_htm_offset;

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

struct htm_status {
	enum htm_state state;
	enum htm_state state2;
	enum htm_error error;
	enum htm_error error2;
	bool mem_size_select;
	uint16_t mem_size;
	uint64_t mem_base;
	uint64_t mem_last;
	uint64_t mem_last2;
	uint64_t raw;
};

static struct htm *check_and_convert(struct pdbg_target *target)
{
	if (!pdbg_target_is_class(target, "nhtm") &&
	    !pdbg_target_is_class(target, "chtm"))
	    return NULL;

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return NULL;

	return target_to_htm(target);
}

int htm_start(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	if (!htm)
		return -1;

	if (!htm->start) {
		PR_ERROR("start() not implemented for the target\n");
		return -1;
	}

	return htm->start(htm);
}

int htm_stop(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	if (!htm)
		return -1;

	if (!htm->stop) {
		PR_ERROR("stop() not implemented for the target\n");
		return -1;
	}

	return htm->stop(htm);
}

int htm_status(struct pdbg_target *target)
{
	struct htm *htm = check_and_convert(target);

	if (!htm)
		return -1;

	if (!htm->status) {
		PR_ERROR("status() not implemented for the target\n");
		return -1;
	}

	return htm->status(htm);
}

int htm_dump(struct pdbg_target *target, char *filename)
{
	struct htm *htm = check_and_convert(target);

	if (!htm || !filename)
		return -1;

	if (!htm->dump) {
		PR_ERROR("dump() not implemented for the target\n");
		return -1;
	}

	return htm->dump(htm, filename);
}

int htm_record(struct pdbg_target *target, char *filename)
{
	struct htm *htm = check_and_convert(target);

	if (!htm || !filename)
		return -1;

	if (!htm->record) {
		PR_ERROR("record() not implemented for the target\n");
		return -1;
	}

	return htm->record(htm, filename);
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

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm")) {
		switch (status->raw & HTM2_STATUS_ERROR_MASK) {
		case HTM2_STATUS_CRESP_OV:
			status->error2 = CRESP_OV;
			break;
		case HTM2_STATUS_BUF_WAIT:
			status->error2 = BUF_WAIT;
			break;
		case HTM2_STATUS_TRIG_DROPPED:
			status->error2 = TRIGGER_DROPPED;
			break;
		case HTM2_STATUS_ADDR_ERROR:
			status->error2 = ADDR_ERROR;
			break;
		case HTM2_STATUS_REC_DROPPED:
			status->error2 = RECORD_DROPPED;
			break;
		case HTM2_STATUS_SCOM_ERROR:
			status->error2 = SCOM_ERROR;
			break;
		case HTM2_STATUS_PARITY_ERROR:
			status->error2 = PARITY_ERROR;
			break;
		case HTM2_STATUS_INVALID_CRESP:
			status->error2 = INVALID_CRESP;
			break;
		default:
			status->error2 = NONE;
		}
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

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm")) {
		switch (status->raw & HTM2_STATUS_STATE_MASK) {
		case HTM2_STATUS_REPAIR:
			status->state2 = REPAIR;
			break;
		case HTM2_STATUS_INIT:
			status->state2 = INIT;
			break;
		case HTM2_STATUS_PREREQ:
			status->state2 = PREREQ;
			break;
		case HTM2_STATUS_READY:
			status->state2 = READY;
			break;
		case HTM2_STATUS_TRACING:
			status->state2 = TRACING;
			break;
		case HTM2_STATUS_PAUSED:
			status->state2 = PAUSED;
			break;
		case HTM2_STATUS_FLUSH:
			status->state2 = FLUSH;
			break;
		case HTM2_STATUS_COMPLETE:
			status->state2 = COMPLETE;
			break;
		case HTM2_STATUS_ENABLE:
			status->state2 = ENABLE;
			break;
		case HTM2_STATUS_STAMP:
			status->state2 = STAMP;
			break;
		default:
			status->state2 = UNINITIALIZED;
		}
	}

	if (HTM_ERR(pib_read(&htm->target, HTM_MEMORY_CONF, &val)))
		return -1;

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm")) {
		status->mem_size_select = val & HTM_MEM_SIZE_SMALL_P10;
		status->mem_base = val & HTM_MEM_BASE_P10;
	} else {
		status->mem_size_select = val & HTM_MEM_SIZE_SMALL;
		status->mem_base = val & HTM_MEM_BASE;
	}
	status->mem_size = GETFIELD(HTM_MEM_SIZE,val);

	if (HTM_ERR(pib_read(&htm->target, HTM_LAST_ADDRESS, &status->mem_last)))
		return -1;

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm")) {
		if (HTM_ERR(pib_read(&htm->target, HTM2_LAST_ADDRESS, &status->mem_last2)))
			return -1;
	}

	return 0;
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
		P_INFO("Unexpected status on HTM start trigger PMISC command: 0x%"
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

static int configure_chtm(struct htm *htm, bool wrap)
{
	uint64_t val;

	if (!pdbg_target_is_class(&htm->target, "chtm"))
		return 0;

	if (HTM_ERR(configure_debugfs_memtrace(htm)))
		return -1;

	val = wrap ? HTM_MODE_WRAP : 0;
	if (HTM_ERR(pib_write(&htm->target, HTM_COLLECTION_MODE,
		HTM_MODE_ENABLE | val)))
		return -1;

	if (htm->configure && htm->configure(htm) < 0)
		return -1;

	return 0;
}

static int do_configure_chtm_p8(struct htm *htm)
{
	uint64_t hid0, ncu;
	struct pdbg_target *core;

	core = pdbg_target_require_parent("core", &htm->target);
	if (HTM_ERR(pib_read(core, HID0_REGISTER, &hid0)))
		return -1;
	hid0 |= HID0_TRACE_BITS;
	if (HTM_ERR(pib_write(core, HID0_REGISTER, hid0)))
		return -1;

	if (HTM_ERR(pib_read(core, NCU_MODE_REGISTER, &ncu)))
		return -1;
	ncu |= NCU_MODE_HTM_ENABLE;
	if (HTM_ERR(pib_write(core, NCU_MODE_REGISTER, ncu)))
		return -1;

	return 0;
}

static int deconfigure_chtm(struct htm *htm)
{

	if (!pdbg_target_is_class(&htm->target, "chtm"))
		return 0;

	if (htm->deconfigure && htm->deconfigure(htm) < 0)
		return -1;

	if (HTM_ERR(pib_write(&htm->target, HTM_COLLECTION_MODE,0)))
		return -1;

//	FIXME this needs kernel work to happen
//	if (HTM_ERR(deconfigure_debugfs_memtrace(htm)))
//		return -1;
	return 0;
}

static int do_deconfigure_chtm_p8(struct htm *htm)
{
	struct pdbg_target *core;
	uint64_t ncu, hid0;

	core = pdbg_target_require_parent("core", &htm->target);
	if (HTM_ERR(pib_read(core, NCU_MODE_REGISTER, &ncu)))
		return -1;
	ncu &= ~NCU_MODE_HTM_ENABLE;
	if (HTM_ERR(pib_write(core, NCU_MODE_REGISTER, ncu)))
		return -1;

	if (HTM_ERR(pib_read(core, HID0_REGISTER, &hid0)))
		return -1;
	hid0 &= ~(HID0_TRACE_BITS);
	if (HTM_ERR(pib_write(core, HID0_REGISTER, hid0)))
		return -1;

	return 0;
}

static int configure_nhtm(struct htm *htm, bool wrap)
{
	uint64_t val;

	if (!pdbg_target_is_class(&htm->target, "nhtm"))
		return 0;

	if (HTM_ERR(configure_debugfs_memtrace(htm)))
		return -1;

	/*
	 * The constant is the VGTARGET field, taken from a cronus
	 * booted system which presumably set it up correctly
	 */
	val = wrap ? HTM_MODE_WRAP : 0;
	if (HTM_ERR(pib_write(&htm->target, HTM_COLLECTION_MODE,
					HTM_MODE_ENABLE |
					NHTM_MODE_CRESP_PRECISE |
					val |
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

	if (pdbg_target_compatible(&htm->target, "ibm,power9-nhtm")) {
		if (HTM_ERR(pib_read(&htm->target, NHTM_FLEX_MUX, &val)))
			return -1;

		if (GETFIELD(NHTM_FLEX_MUX_MASK, val) != NHTM_FLEX_DEFAULT) {
			PR_ERROR("The HTM Flex wasn't default value\n");
			return -1;
		}
	}

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm")) {
		if (HTM_ERR(pib_read(&htm->target, NHTM_FLEX_MUX, &val)))
			return -1;

		if (GETFIELD(NHTM_FLEX_MUX_MASK_P10, val) != NHTM_FLEX_DEFAULT_P10) {
			PR_ERROR("The HTM Flex wasn't default value\n");
			return -1;
		}

		if (HTM_ERR(pib_write(&htm->target, NHTM_ADDR_PAT, 0)))
			return -1;

		if (HTM_ERR(pib_write(&htm->target, NHTM_ADDR_MASK, NHTM_ADDR_MASK_MASK)))
			return -1;

		if (HTM_ERR(pib_write(&htm->target, NHTM_STOP_FILTER,
				      NHTM_STOP_FILTER_TTAG_MASK |
				      NHTM_STOP_FILTER_TTYPE_MASK |
				      NHTM_STOP_FILTER_CRESP_MASK)))
			return -1;

		if (HTM_ERR(pib_write(&htm->target, NHTM_STOP_FILT_ADDR_PAT, 0)))
			return -1;

		if (HTM_ERR(pib_write(&htm->target, NHTM_STOP_FILT_ADDR_MASK,
				      NHTM_STOP_FILT_ADDR_MASK_MASK)))
			return -1;
	}

	return 0;
}

static int deconfigure_nhtm(struct htm *htm)
{
	if (!pdbg_target_is_class(&htm->target, "nhtm"))
		return 0;
	// FIXME: write and test this
	return 0;
}

static int is_startable(struct htm *htm, struct htm_status *status)
{
	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm"))
		return ((status->state == READY || status->state == PAUSED) &&
			(status->state2 == READY || status->state2 == PAUSED));
	else
		return (status->state == READY || status->state == PAUSED);
}

static char *get_debugfs_file(struct htm *htm, const char *file)
{
	struct pdbg_target *pib;
	uint32_t index;
	char *filename;

	pib = pdbg_target_parent("pib", &htm->target);
	assert(pib);
	index = pdbg_target_index(pib);
	if (asprintf(&filename, "%s/%08x/%s", DEBUGFS_MEMTRACE, index, file) == -1) {
		PR_ERROR("Couldn't asprintf() '%s/%08x/size': %m\n",
				DEBUGFS_MEMTRACE, index);
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

static int configure_memory(struct htm *htm)
{
	uint64_t size, base, val, small, mem_size;
	int shift;

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

	/*
	 * HTMSC_MEM_SIZE_SMALL:
	 * 0 = Trace Mem Size from 512M to 256G
	 * 1 = Trace Mem Size from 16M to 8G
	 */
	if (size < 16777216UL) {
		PR_ERROR("memtrace size must be atleast 16MB. Currently:%"
			 PRIx64 "\n", size);
		return -1;
	}
	if (size > 274877906944UL) {
		PR_ERROR("memtrace size must be smaller than 256GB. Currently:%"
			 PRIx64 "\n", size);
		return -1;
	}

	/*
	 * The hardware has two sizes, small and large
	 *   small: 16M  - 8G
	 *   large: 512M - 256G
	 * We use small for 16M-256, then large for 512M-256G
	 *
	 * Hardware buffer sizes:
	 * Large  Small    HTMSC_MEM_SIZE encoding
	 * 512M   16M        0b000000000
	 * 1G     32M        0b000000001
	 * 2G     64M        0b000000011
	 * 4G     128M       0b000000111
	 * 8G     256M       0b000001111
	 * 16G    512M       0b000011111
	 * 32G    1G         0b000111111
	 * 64G    2G         0b001111111
	 * 128G   4G         0b011111111
	 * 256G   8G         0b111111111
	 */
	small = 0;
	if (size < 512*1024*1024)
		small = 1;
	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm"))
		val = SETFIELD(HTM_MEM_SIZE_SMALL_P10, val, small);
	else
		val = SETFIELD(HTM_MEM_SIZE_SMALL, val, small);
	shift = 29; /* large */
	if (small)
		shift = 24;
	mem_size = (size >> shift) - 1;
	val = SETFIELD(HTM_MEM_SIZE, val, mem_size);

	/*
	 * Clear out the base
	 * Don't use SETFIELD to write the base in since the value is
	 * already shifted correct as read from the kernel, or'ing it
	 * in works fine.
	 */
	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm"))
		val = SETFIELD(HTM_MEM_BASE_P10, val, 0);
	else
		val = SETFIELD(HTM_MEM_BASE, val, 0);
	val |= base;

	if (HTM_ERR(pib_write(&htm->target, HTM_MEMORY_CONF, val)))
		return -1;

	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_RESET)))
		return -1;

	return 0;
}

static int do_htm_reset(struct htm *htm, bool wrap)
{
	struct htm_status status;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (configure_nhtm(htm, wrap) < 0)
		return -1;
	if (configure_chtm(htm, wrap) < 0)
		return -1;

	if (HTM_ERR(configure_memory(htm)))
		return -1;

	return 1;
}

/* Stolen from p8chip.c */
#define RAS_MODE_REG			0x1
#define  MR_THREAD_IN_DEBUG		PPC_BIT(43)
static int do_post_configure_chtm_p8(struct htm *htm)
{
	struct pdbg_target *target;
	struct pdbg_target *core;
	uint64_t reg;

	/* NHTM doesn't have a core as a parent but donesn't need this
	 * bit toggled */
	core = pdbg_target_parent("core", &htm->target);
	if (!core)
		return 0; /* nhtm case */

	pdbg_for_each_target("thread", core, target) {
		if (pdbg_target_index(target) == 0) {
			/* Need to set this bit to ensure HTM starts */
			pib_read (target, RAS_MODE_REG, &reg);
			pib_write(target, RAS_MODE_REG, reg | MR_THREAD_IN_DEBUG);
			pib_write(target, RAS_MODE_REG, reg);
		}
	}
	return 0;
}

static int __do_htm_start(struct htm *htm, bool wrap)
{
	struct htm_status status;

	if (do_htm_reset(htm, wrap) < 0)
		return -1;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (!is_startable(htm, &status)) {
		PR_ERROR("HTM not in a startable state!\n");
		return -1;
	}

	PR_INFO("* Sending START trigger to HTM\n");
	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_MARK_VALID)))
		return -1;

	if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_START)))
		return -1;

	if (htm->post_configure && htm->post_configure(htm))
		return -1;

	/*
	 * Instead of the HTM_TRIG_START, this is where you might want
	 * to call do_adu_magic()
	 * for_each_child_target("mem", core, do_adu_magic, NULL, NULL);
	 * see what I mean?
	 */

	return 1;
}

static int do_htm_start(struct htm *htm)
{
	return __do_htm_start(htm, true);
}

static int do_htm_stop(struct htm *htm)
{
	struct htm_status status;
	get_status(htm, &status);
	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (status.state == UNINITIALIZED) {
		PR_INFO("* Skipping STOP trigger, HTM appears uninitialized\n");
		return -1;
	}
	if (status.state == TRACING) {
		PR_INFO("* Sending STOP trigger to HTM\n");
		if (HTM_ERR(pib_write(&htm->target, HTM_SCOM_TRIGGER, HTM_TRIG_STOP)))
			return -1;
	} else {
		PR_INFO("* Skipping STOP trigger, HTM is not running\n");
	}

	if (deconfigure_chtm(htm) < 0)
		return -1;
	if (deconfigure_nhtm(htm) < 0)
		return -1;

	return 1;
}

static uint64_t htm_trace_size(struct htm_status *status)
{
	uint64_t size;
	uint64_t mem_size = status->mem_size;

	if (status->mem_size_select)
		size = 16;
	else
		size = 512;

	while (mem_size) {
		size <<= 1;
		mem_size >>= 1;
	}

	return size << 20;
}

static bool htm_complete(struct htm_status *status)
{
	return (status->state == COMPLETE);
}

static int htm_wait_complete(struct htm *htm)
{
	struct htm_status status;

	while (1) {
		if (HTM_ERR(get_status(htm, &status)))
			return -1;
		PR_DEBUG("loop curr:0x%016" PRIx64 "\n", status.mem_last);
		if (htm_complete(&status))
			break;
		usleep(100000);
	}
	return 0;
}

static int do_htm_status(struct htm *htm)
{
	struct htm_status status;
	uint64_t val, total;
	int i, regs = 9;

	if (pdbg_target_compatible(&htm->target, "ibm,power9-nhtm"))
		regs++;

	PR_INFO("HTM register dump:\n");
	for (i = 0; i < regs; i++) {
		if (HTM_ERR(pib_read(&htm->target, i, &val))) {
			PR_ERROR("Couldn't read HTM reg: %d\n", i);
			continue;
		}

		PR_INFO(" %d: 0x%016" PRIx64 "\n", i, val);
	}

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	total = htm_trace_size(&status);

	PR_DEBUG("HTM status : 0x%016" PRIx64 "\n", status.raw);
	printf("State: ");
	switch (status.state) {
	case INIT:
		printf("INIT");
		break;
	case PREREQ:
		printf("PREREQ");
		break;
	case READY:
		printf("READY");
		break;
	case TRACING:
		printf("TRACING");
		break;
	case PAUSED:
		printf("PAUSED");
		break;
	case FLUSH:
		printf("FLUSH");
		break;
	case COMPLETE:
		printf("COMPLETE");
		break;
	case ENABLE:
		printf("ENABLE");
		break;
	case STAMP:
		printf("STAMP");
		break;
	default:
		printf("UNINITIALIZED");
	}
	printf("\n");

	printf("addr:0x%016" PRIx64 "\n", status.mem_base);
	printf("size:0x%016" PRIx64 " ", total);
	if (total >= 0x20000000)
		printf("[ %" PRIu64 "GB ]", total >> 30);
	else
		printf("[ %" PRIu64 "MB ]", total >> 20);
	printf("\n");
	printf("curr:0x%016" PRIx64 "\n", status.mem_last);

	return 1;
}

#define COPY_BUF_SIZE getpagesize()
static int copy_file(int output, int input, uint64_t size)
{
	char *buf;
	size_t r;

	buf = malloc(COPY_BUF_SIZE);
	if (!buf) {
		PR_ERROR("Can't malloc buffer\n");
		return -1;
	}

	while (size) {
		r = read(input, buf, MIN(COPY_BUF_SIZE, size));
		if (r == -1) {
			PR_ERROR("Failed to read\n");
			goto out;
		}
		if (r == 0) {
			PR_ERROR("EOF\n");
			goto out;
		}

		if (write(output, buf, r) != r) {
			PR_ERROR("Short write!\n");
			goto out;
		}
		size -= r;
	}

	return 0;

out:
	free(buf);
	return -1;

}

static int do_htm_dump(struct htm *htm, char *filename)
{
	char *trace_file;
	struct htm_status status;
	uint64_t last, end, trace_size, last2 = 0;
	int trace_fd, dump_fd;
	uint64_t eyecatcher;
	size_t r;
	bool wrapped;

	if (!filename)
		return -1;

	if (HTM_ERR(get_status(htm, &status)))
		return -1;

	if (status.state != COMPLETE) {
		PR_INFO("* Skipping DUMP tigger, HTM is not in complete state\n");
		return -1;
	}

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm") && status.state2 != COMPLETE) {
		PR_INFO("* Skipping DUMP tigger, HTM2 is not in complete state\n");
		return -1;
	}

	trace_file = get_debugfs_file(htm, "trace");
	if (!trace_file)
		return -1;

	trace_fd = open(trace_file, O_RDWR);
	if (trace_fd == -1) {
		PR_ERROR("Failed to open %s: %m\n", trace_file);
		r = trace_fd;
		goto out3;
	}

	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm")) {
		lseek(trace_fd, 8, SEEK_SET);
		r = read(trace_fd, &eyecatcher, 8);
	} else {
		r = read(trace_fd, &eyecatcher, 4);
	}
	if (r == -1) {
		PR_ERROR("Failed to read from %s: %m\n", trace_file);
		goto out2;
	}
	wrapped = true;
	if (eyecatcher == 0x00f0efac)
		wrapped = false;
	last = status.mem_last - status.mem_base;
	if (pdbg_target_compatible(&htm->target, "ibm,power10-nhtm"))
		last2 = status.mem_last2 - status.mem_base;
	end = htm_trace_size(&status);
	trace_size = wrapped ? end : last;

	printf("Dumping %" PRIi64 " MB to %s\n", trace_size >> 20, filename);

	dump_fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (dump_fd == -1) {
		PR_ERROR("Failed to open %s: %m\n", filename);
		r = dump_fd;
		goto out2;
	}

	/*
	 * Trace buffer:
         *   ------------------------------------------------
	 *   |                              |               |
         *   ------------------------------------------------
	 * start                          last             end
	 *
	 * If the trace buffer has wrapped, the start of the trace is
	 * after the last written value. In this case we need to copy
	 * last -> end to dump file.  Then copy start -> last to the
	 * dump file.
	 */
	if (wrapped) {
		/* Copy last -> end first */
		r =  lseek(trace_fd, last, SEEK_SET);
		if (r == -1)
			goto out1;
		r = copy_file(dump_fd, trace_fd, end - last);
		if (r)
			goto out1;
	}

	/* Copy start -> last */
	r = lseek(trace_fd, 0, SEEK_SET);
	if (r == -1)
		goto out1;
	r = copy_file(dump_fd, trace_fd, MAX(last2, last));
	if (r)
		goto out1;
	r = 1;

out1:
	close(dump_fd);
out2:
	close(trace_fd);
out3:
	free(trace_file);
	return r;
}

static int do_htm_record(struct htm *htm, char *filename)
{
	if (__do_htm_start(htm, false) < 0)
		return -1;

	if (htm_wait_complete(htm))
		return -1;

	if (do_htm_stop(htm) < 0)
		return -1;

	if (do_htm_dump(htm, filename) < 0)
		return -1;


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

	if (pdbg_target_compatible(target, "ibm,power9-nhtm")) {
		pib_read(target, NHTM_FLEX_MUX, &val);
		if (GETFIELD(NHTM_FLEX_MUX_MASK, val) != NHTM_FLEX_DEFAULT) {
			PR_DEBUG("FLEX_MUX not default\n");
			return -1;
		}
	}

	switch (pdbg_get_proc()) {
	case PDBG_PROC_P10:
		dual_htm_offset = 0x01;
		break;
	default:
		break;
	}

	return 0;
}

static int chtm_probe(struct pdbg_target *target)
{
	return is_debugfs_memtrace_ok() && is_debugfs_scom_ok() ? 0 : -1;
}

static struct htm p8_nhtm = {
	.target = {
		.name =	"POWER8 Nest HTM",
		.compatible = "ibm,power8-nhtm",
		.class = "nhtm",
		.probe = nhtm_probe,
	},
	.start = do_htm_start,
	.stop = do_htm_stop,
	.record = do_htm_record,
	.status = do_htm_status,
	.dump = do_htm_dump,
};
DECLARE_HW_UNIT(p8_nhtm);

static struct htm p9_nhtm = {
	.target = {
		.name =	"POWER9 Nest HTM",
		.compatible = "ibm,power9-nhtm",
		.class = "nhtm",
		.probe = nhtm_probe,
	},
	.start = do_htm_start,
	.stop = do_htm_stop,
	.record = do_htm_record,
	.status = do_htm_status,
	.dump = do_htm_dump,
};
DECLARE_HW_UNIT(p9_nhtm);

static struct htm p8_chtm = {
	.target = {
		.name = "POWER8 Core HTM",
		.compatible = "ibm,power8-chtm",
		.class = "chtm",
		.probe = chtm_probe,
	},
	.start = do_htm_start,
	.stop = do_htm_stop,
	.record = do_htm_record,
	.status = do_htm_status,
	.dump = do_htm_dump,
	.configure = do_configure_chtm_p8,
	.deconfigure = do_deconfigure_chtm_p8,
	.post_configure = do_post_configure_chtm_p8,
};
DECLARE_HW_UNIT(p8_chtm);

static struct htm p10_nhtm = {
	.target = {
		.name = "POWER10 Nest HTM",
		.compatible = "ibm,power10-nhtm",
		.class = "nhtm",
		.probe = nhtm_probe,
	},
	.start = do_htm_start,
	.stop = do_htm_stop,
	.record = do_htm_record,
	.status = do_htm_status,
	.dump = do_htm_dump,
};
DECLARE_HW_UNIT(p10_nhtm);

__attribute__((constructor))
static void register_htm(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p8_nhtm_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p9_nhtm_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p10_nhtm_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &p8_chtm_hw_unit);
}
