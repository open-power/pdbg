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

static int sbefifo_op_getmem(struct mem *sbefifo_mem,
			     uint64_t addr, uint8_t *data, uint64_t size,
			     uint8_t block_size, bool ci)
{
	struct sbefifo *sbefifo = target_to_sbefifo(sbefifo_mem->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t *out;
	uint32_t len;
	uint16_t flags;
	int rc;

	if (size > 0xffffffff) {
		PR_ERROR("sbefifo: Invalid size for getmem\n");
		return EINVAL;
	}

	len = size & 0xffffffff;

	PR_NOTICE("sbefifo: getmem addr=0x%016" PRIx64 ", len=%u\n",
		  addr, len);

	flags = SBEFIFO_MEMORY_FLAG_PROC;
	if (ci)
		flags |= SBEFIFO_MEMORY_FLAG_CI;

	rc = sbefifo_mem_get(sctx, addr, len, flags, &out);
	if (rc)
		return rc;

	pdbg_progress_tick(len, len);

	memcpy(data, out, len);
	free(out);

	return 0;
}

static int sbefifo_op_putmem(struct mem *sbefifo_mem,
			     uint64_t addr, uint8_t *data, uint64_t size,
			     uint8_t block_size, bool ci)
{
	struct sbefifo *sbefifo = target_to_sbefifo(sbefifo_mem->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint32_t len;
	uint16_t flags;
	int rc;

	if (size > 0xffffffff) {
		PR_ERROR("sbefifo: Invalid size for putmem\n");
		return EINVAL;
	}

	len = size & 0xffffffff;

	PR_NOTICE("sbefifo: putmem addr=0x%016"PRIx64", len=%u\n", addr, len);

	flags = SBEFIFO_MEMORY_FLAG_PROC;
	if (ci)
		flags |= SBEFIFO_MEMORY_FLAG_CI;

	rc = sbefifo_mem_put(sctx, addr, data, len, flags);
	if (rc)
		return rc;

	pdbg_progress_tick(len, len);

	return 0;
}

static int sbefifo_op_getmem_pba(struct mem *sbefifo_mem,
				 uint64_t addr, uint8_t *data, uint64_t size,
				 uint8_t block_size, bool ci)
{
	struct sbefifo *sbefifo = target_to_sbefifo(sbefifo_mem->target.parent);
	uint8_t *out;
	uint32_t len;
	uint16_t flags;
	int rc;

	if (size > 0xffffffff) {
		PR_ERROR("sbefifo: Invalid size for getmempba\n");
		return EINVAL;
	}

	len = size & 0xffffffff;

	PR_NOTICE("sbefifo: getmempba addr=0x%016" PRIx64 ", len=%u\n", addr, len);

	flags = SBEFIFO_MEMORY_FLAG_PBA;
	if (ci)
		flags |= SBEFIFO_MEMORY_FLAG_CI;

	rc = sbefifo_mem_get(sbefifo->sf_ctx, addr, len, flags, &out);
	if (rc)
		return rc;

	pdbg_progress_tick(len, len);

	memcpy(data, out, len);
	free(out);

	return 0;
}

static int sbefifo_op_putmem_pba(struct mem *sbefifo_mem,
				 uint64_t addr, uint8_t *data, uint64_t size,
				 uint8_t block_size, bool ci)
{
	struct sbefifo *sbefifo = target_to_sbefifo(sbefifo_mem->target.parent);
	uint32_t len;
	uint16_t flags;
	int rc;

	if (size > 0xffffffff) {
		PR_ERROR("sbefifo: Invalid size for putmempba\n");
		return EINVAL;
	}

	len = size & 0xffffffff;

	PR_NOTICE("sbefifo: putmempba addr=0x%016"PRIx64", len=%u\n", addr, len);

	flags = SBEFIFO_MEMORY_FLAG_PBA;
	if (ci)
		flags |= SBEFIFO_MEMORY_FLAG_CI;

	rc = sbefifo_mem_put(sbefifo->sf_ctx, addr, data, len, flags);
	if (rc)
		return rc;

	pdbg_progress_tick(len, len);

	return 0;
}

static uint32_t sbefifo_op_ffdc_get(struct chipop *chipop, const uint8_t **ffdc, uint32_t *ffdc_len)
{
	struct sbefifo *sbefifo = target_to_sbefifo(chipop->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	return sbefifo_ffdc_get(sctx, ffdc, ffdc_len);
}

static int sbefifo_op_istep(struct chipop *chipop,
			    uint32_t major, uint32_t minor)
{
	struct sbefifo *sbefifo = target_to_sbefifo(chipop->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	PR_NOTICE("sbefifo: istep %u.%u\n", major, minor);

	return sbefifo_istep_execute(sctx, major & 0xff, minor & 0xff);
}

static int sbefifo_op_mpipl_continue(struct chipop *chipop)
{
	struct sbefifo *sbefifo = target_to_sbefifo(chipop->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	return sbefifo_mpipl_continue(sctx);
}

static int sbefifo_op_mpipl_enter(struct chipop *chipop)
{
	struct sbefifo *sbefifo = target_to_sbefifo(chipop->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	return sbefifo_mpipl_enter(sctx);
}

static int sbefifo_op_control(struct chipop *chipop,
			      uint32_t core_id, uint32_t thread_id,
			      uint32_t oper)
{
	struct sbefifo *sbefifo = target_to_sbefifo(chipop->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t mode = 0;

	/* Enforce special-wakeup for thread stop and sreset */
	if ((oper & 0xf) == SBEFIFO_INSN_OP_STOP ||
	    (oper & 0xf) == SBEFIFO_INSN_OP_SRESET)
		mode = 0x2;

	PR_NOTICE("sbefifo: control c:0x%x, t:0x%x, op:%u mode:%u\n", core_id, thread_id, oper, mode);

	return sbefifo_control_insn(sctx, core_id & 0xff, thread_id & 0xff, oper & 0xff, mode);
}

static int sbefifo_op_thread_start(struct chipop *chipop,
				   uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(chipop, core_id, thread_id, SBEFIFO_INSN_OP_START);
}

static int sbefifo_op_thread_stop(struct chipop *chipop,
				  uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(chipop, core_id, thread_id, SBEFIFO_INSN_OP_STOP);
}

static int sbefifo_op_thread_step(struct chipop *chipop,
				  uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(chipop, core_id, thread_id, SBEFIFO_INSN_OP_STEP);
}

static int sbefifo_op_thread_sreset(struct chipop *chipop,
				    uint32_t core_id, uint32_t thread_id)
{
	return sbefifo_op_control(chipop, core_id, thread_id, SBEFIFO_INSN_OP_SRESET);
}

static struct sbefifo_context *sbefifo_op_get_context(struct sbefifo *sbefifo)
{
	return sbefifo->sf_ctx;
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

static void sbefifo_release(struct pdbg_target *target)
{
	struct sbefifo *sf = target_to_sbefifo(target);

	sbefifo_disconnect(sf->sf_ctx);
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

static struct mem sbefifo_pba = {
	.target = {
		.name = "SBE FIFO Chip-op based memory access",
		.compatible = "ibm,sbefifo-mem-pba",
		.class = "mem",
	},
	.read = sbefifo_op_getmem_pba,
	.write = sbefifo_op_putmem_pba,
};
DECLARE_HW_UNIT(sbefifo_pba);

static struct chipop sbefifo_chipop = {
	.target = {
		.name = "SBE FIFO Chip-op engine",
		.compatible = "ibm,sbefifo-chipop",
		.class = "chipop",
	},
	.ffdc_get = sbefifo_op_ffdc_get,
	.istep = sbefifo_op_istep,
	.mpipl_enter = sbefifo_op_mpipl_enter,
	.mpipl_continue = sbefifo_op_mpipl_continue,
	.thread_start = sbefifo_op_thread_start,
	.thread_stop = sbefifo_op_thread_stop,
	.thread_step = sbefifo_op_thread_step,
	.thread_sreset = sbefifo_op_thread_sreset,
};
DECLARE_HW_UNIT(sbefifo_chipop);

static struct sbefifo kernel_sbefifo = {
	.target = {
		.name =	"Kernel based FSI SBE FIFO",
		.compatible = "ibm,kernel-sbefifo",
		.class = "sbefifo",
		.probe = sbefifo_probe,
		.release = sbefifo_release,
	},
	.get_sbefifo_context = sbefifo_op_get_context,
};
DECLARE_HW_UNIT(kernel_sbefifo);

__attribute__((constructor))
static void register_sbefifo(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &kernel_sbefifo_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_chipop_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_mem_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_pba_hw_unit);
}
