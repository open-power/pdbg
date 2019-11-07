/* Copyright 2016 IBM Corp.
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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>

#include <libsbefifo/libsbefifo.h>

#include "hwunit.h"
#include "debug.h"

static uint32_t sbefifo_op_ffdc_get(struct sbefifo *sbefifo, const uint8_t **ffdc, uint32_t *ffdc_len)
{
	return sbefifo_ffdc_get(sbefifo->sf_ctx, ffdc, ffdc_len);
}

static int sbefifo_op_istep(struct sbefifo *sbefifo,
			    uint32_t major, uint32_t minor)
{
	PR_NOTICE("sbefifo: istep %u.%u\n", major, minor);

	return sbefifo_istep_execute(sbefifo->sf_ctx, major & 0xff, minor & 0xff);
}

static int sbefifo_op_getmem(struct mem *sbefifo_mem,
			     uint64_t addr, uint8_t *data, uint64_t size,
			     uint8_t block_size, bool ci)
{
	struct sbefifo *sbefifo = target_to_sbefifo(sbefifo_mem->target.parent);
	uint8_t *out;
	uint64_t start_addr, end_addr;
	uint32_t align, offset, len;
	uint16_t flags;
	int rc;

	align = 8;

	if (block_size && block_size != 8) {
		PR_ERROR("sbefifo: Only 8 byte block sizes are supported\n");
		return -1;
	};

	start_addr = addr & (~(uint64_t)(align-1));
	end_addr = (addr + size + (align-1)) & (~(uint64_t)(align-1));

	if (end_addr - start_addr > UINT32_MAX) {
		PR_ERROR("sbefifo: size too large\n");
		return -EINVAL;
	}

	offset = addr - start_addr;
	len = end_addr - start_addr;

	PR_NOTICE("sbefifo: getmem addr=0x%016" PRIx64 ", len=%u\n",
		  start_addr, len);

	flags = SBEFIFO_MEMORY_FLAG_PROC;
	if (ci)
		flags |= SBEFIFO_MEMORY_FLAG_CI;

	rc = sbefifo_mem_get(sbefifo->sf_ctx, start_addr, len, flags, &out);

	pdbg_progress_tick(len, len);

	if (rc)
		return rc;

	memcpy(data, out+offset, size);
	free(out);

	return 0;
}

static int sbefifo_op_putmem(struct mem *sbefifo_mem,
			     uint64_t addr, uint8_t *data, uint64_t size,
			     uint8_t block_size, bool ci)
{
	struct sbefifo *sbefifo = target_to_sbefifo(sbefifo_mem->target.parent);
	uint32_t align, len;
	uint16_t flags;
	int rc;

	align = 8;

	if (block_size && block_size != 8) {
		PR_ERROR("sbefifo: Only 8 byte block sizes are supported\n");
		return -1;
	};

	if (addr & (align-1)) {
		PR_ERROR("sbefifo: Address must be aligned to %d bytes\n", align);
		return -1;
	}

	if (size & (align-1)) {
		PR_ERROR("sbefifo: Data must be multiple of %d bytes\n", align);
		return -1;
	}

	if (size > UINT32_MAX) {
		PR_ERROR("sbefifo: size too large\n");
		return -1;
	}

	len = size & 0xffffffff;

	PR_NOTICE("sbefifo: putmem addr=0x%016"PRIx64", len=%u\n", addr, len);

	flags = SBEFIFO_MEMORY_FLAG_PROC;
	if (ci)
		flags |= SBEFIFO_MEMORY_FLAG_CI;

	rc = sbefifo_mem_put(sbefifo->sf_ctx, addr, data, len, flags);

	pdbg_progress_tick(len, len);

	return rc;
}

static int sbefifo_op_control(struct sbefifo *sbefifo,
			      uint32_t core_id, uint32_t thread_id,
			      uint32_t oper)
{
	uint8_t mode = 0;

	/* Enforce special-wakeup for thread stop and sreset */
	if ((oper & 0xf) == SBEFIFO_INSN_OP_STOP ||
	    (oper & 0xf) == SBEFIFO_INSN_OP_SRESET)
		mode = 0x2;

	PR_NOTICE("sbefifo: control c:0x%x, t:0x%x, op:%u mode:%u\n", core_id, thread_id, oper, mode);

	return sbefifo_control_insn(sbefifo->sf_ctx, core_id & 0xff, thread_id & 0xff, oper & 0xff, mode);
}

static int sbefifo_op_thread_start(struct sbefifo *sbefifo,
				   uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(sbefifo, core_id, thread_id, SBEFIFO_INSN_OP_START);
}

static int sbefifo_op_thread_stop(struct sbefifo *sbefifo,
				  uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(sbefifo, core_id, thread_id, SBEFIFO_INSN_OP_STOP);
}

static int sbefifo_op_thread_step(struct sbefifo *sbefifo,
				  uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(sbefifo, core_id, thread_id, SBEFIFO_INSN_OP_STEP);
}

static int sbefifo_op_thread_sreset(struct sbefifo *sbefifo,
				    uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(sbefifo, core_id, thread_id, SBEFIFO_INSN_OP_SRESET);
}

static int sbefifo_probe(struct pdbg_target *target)
{
	struct sbefifo *sf = target_to_sbefifo(target);
	const char *sbefifo_path;
	int rc;

	sbefifo_path = pdbg_target_property(target, "device-path", NULL);
	assert(sbefifo_path);

	rc = sbefifo_connect(sbefifo_path, &sf->sf_ctx);
	if (rc) {
		PR_ERROR("Unable to open sbefifo driver %s\n", sbefifo_path);
		return rc;
	}

	return 0;
}

static struct mem sbefifo_mem = {
	.target = {
		.name = "SBE FIFO Chip-op based memory access",
		.compatible = "ibm,sbefifo-mem",
		.class = "mem",
	},
	.read = sbefifo_op_getmem,
	.write = sbefifo_op_putmem,
};
DECLARE_HW_UNIT(sbefifo_mem);

static struct sbefifo kernel_sbefifo = {
	.target = {
		.name =	"Kernel based FSI SBE FIFO",
		.compatible = "ibm,kernel-sbefifo",
		.class = "sbefifo",
		.probe = sbefifo_probe,
	},
	.istep = sbefifo_op_istep,
	.thread_start = sbefifo_op_thread_start,
	.thread_stop = sbefifo_op_thread_stop,
	.thread_step = sbefifo_op_thread_step,
	.thread_sreset = sbefifo_op_thread_sreset,
	.ffdc_get = sbefifo_op_ffdc_get,
};
DECLARE_HW_UNIT(kernel_sbefifo);

__attribute__((constructor))
static void register_sbefifo(void)
{
	pdbg_hwunit_register(&kernel_sbefifo_hw_unit);
	pdbg_hwunit_register(&sbefifo_mem_hw_unit);
}
