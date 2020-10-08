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
#include "sprs.h"

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

static int sbefifo_op_mpipl_get_ti_info(struct chipop *chipop, uint8_t **data, uint32_t *data_len)
{
	struct sbefifo *sbefifo = target_to_sbefifo(chipop->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	return sbefifo_mpipl_get_ti_info(sctx, data, data_len);
}

static struct sbefifo *pib_to_sbefifo(struct pdbg_target *pib)
{
	struct pdbg_target *target;
	struct sbefifo *sbefifo = NULL;

	pdbg_for_each_class_target("sbefifo", target) {
		if (pdbg_target_index(target) == pdbg_target_index(pib)) {
			sbefifo = target_to_sbefifo(target);
			break;
		}
	}

	if (sbefifo == NULL)
		assert(sbefifo);

	return sbefifo;
}

static int sbefifo_pib_read(struct pib *pib, uint64_t addr, uint64_t *val)
{
	struct sbefifo *sbefifo = pib_to_sbefifo(&pib->target);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	return sbefifo_scom_get(sctx, addr, val);
}

static int sbefifo_pib_write(struct pib *pib, uint64_t addr, uint64_t val)
{
	struct sbefifo *sbefifo = pib_to_sbefifo(&pib->target);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	return sbefifo_scom_put(sctx, addr, val);
}

static int sbefifo_pib_thread_op(struct pib *pib, uint32_t oper)
{
	struct sbefifo *sbefifo = target_to_sbefifo(pib->target.parent);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint32_t core_id, thread_id;
	uint8_t mode = 0;

	/*
	 * core_id = 0xff (all SMT4 cores)
	 * thread_id = 0xf (all 4 threads in the SMT4 core)
	 */
	core_id = 0xff;
	thread_id = 0xf;

	/* Enforce special-wakeup for thread stop and sreset */
	if ((oper & 0xf) == SBEFIFO_INSN_OP_STOP ||
	    (oper & 0xf) == SBEFIFO_INSN_OP_SRESET)
		mode = 0x2;

	return sbefifo_control_insn(sctx, core_id, thread_id, oper, mode);
}

static int sbefifo_pib_thread_start(struct pib *pib)
{
	return sbefifo_pib_thread_op(pib, SBEFIFO_INSN_OP_START);
}

static int sbefifo_pib_thread_stop(struct pib *pib)
{
	return sbefifo_pib_thread_op(pib, SBEFIFO_INSN_OP_STOP);
}

static int sbefifo_pib_thread_step(struct pib *pib, int count)
{
	int i, rc = 0;

	for (i = 0; i < count; i++)
		rc |= sbefifo_pib_thread_op(pib, SBEFIFO_INSN_OP_STEP);

	return rc;
}

static int sbefifo_pib_thread_sreset(struct pib *pib)
{
	return sbefifo_pib_thread_op(pib, SBEFIFO_INSN_OP_SRESET);
}

static int sbefifo_thread_probe(struct pdbg_target *target)
{
	struct thread *thread = target_to_thread(target);

	thread->id = pdbg_target_index(target);

	return 0;
}

static void sbefifo_thread_release(struct pdbg_target *target)
{
}

static int sbefifo_thread_op(struct thread *thread, uint32_t oper)
{
	struct pdbg_target *chiplet =
		pdbg_target_require_parent("chiplet", &thread->target);
	struct pdbg_target *pib = pdbg_target_require_parent("pib", chiplet);
	struct sbefifo *sbefifo = pib_to_sbefifo(pib);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint8_t mode = 0;

	/* Enforce special-wakeup for thread stop and sreset */
	if ((oper & 0xf) == SBEFIFO_INSN_OP_STOP ||
	    (oper & 0xf) == SBEFIFO_INSN_OP_SRESET)
		mode = 0x2;

	/* This chip-op requires core-id as pervasive (chiplet) id */
	return sbefifo_control_insn(sctx,
				    pdbg_target_index(chiplet),
				    thread->id,
				    oper,
				    mode);
}

static int sbefifo_thread_start(struct thread *thread)
{
	return sbefifo_thread_op(thread, SBEFIFO_INSN_OP_START);
}

static int sbefifo_thread_stop(struct thread *thread)
{
	return sbefifo_thread_op(thread, SBEFIFO_INSN_OP_STOP);
}

static int sbefifo_thread_step(struct thread *thread, int count)
{
	int i, rc = 0;

	for (i = 0; i < count; i++)
		rc |= sbefifo_thread_op(thread, SBEFIFO_INSN_OP_STEP);

	return rc;
}

static int sbefifo_thread_sreset(struct thread *thread)
{
	return sbefifo_thread_op(thread, SBEFIFO_INSN_OP_SRESET);
}

static int sbefifo_thread_getregs(struct thread *thread, struct thread_regs *regs)
{
	struct pdbg_target *chiplet = pdbg_target_require_parent("chiplet", &thread->target);
	struct pdbg_target *pib = pdbg_target_require_parent("pib", chiplet);
	struct sbefifo *sbefifo = pib_to_sbefifo(pib);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint32_t reg_id[34];
	uint64_t *value;
	int ret, i;

	for (i=0; i<32; i++)
		reg_id[i] = i;

	/* This chip-op requires core-id as pervasive (chiplet) id */
	ret = sbefifo_register_get(sctx,
				   pdbg_target_index(chiplet),
				   thread->id,
				   SBEFIFO_REGISTER_TYPE_GPR,
				   reg_id,
				   32,
				   &value);
	if (ret)
		return ret;

	for (i=0; i<32; i++)
		regs->gprs[i] = value[i];

	free(value);

	reg_id[0] = SPR_NIA;
	reg_id[1] = SPR_MSR;
	reg_id[2] = SPR_CFAR;
	reg_id[3] = SPR_LR;
	reg_id[4] = SPR_CTR;
	reg_id[5] = SPR_TAR;
	reg_id[6] = SPR_CR;
	reg_id[7] = SPR_XER;
	reg_id[8] = SPR_LPCR;
	reg_id[9] = SPR_PTCR;
	reg_id[10] = SPR_LPIDR;
	reg_id[11] = SPR_PIDR;
	reg_id[12] = SPR_HFSCR;
	reg_id[13] = SPR_HDSISR;
	reg_id[14] = SPR_HDAR;
	reg_id[15] = SPR_HSRR0;
	reg_id[16] = SPR_HSRR1;
	reg_id[17] = SPR_HDEC;
	reg_id[18] = SPR_HEIR;
	reg_id[19] = SPR_HID;
	reg_id[20] = SPR_HSPRG0;
	reg_id[21] = SPR_HSPRG1;
	reg_id[22] = SPR_FSCR;
	reg_id[23] = SPR_DSISR;
	reg_id[24] = SPR_DAR;
	reg_id[25] = SPR_SRR0;
	reg_id[26] = SPR_SRR1;
	reg_id[27] = SPR_DEC;
	reg_id[28] = SPR_TB;
	reg_id[29] = SPR_SPRG0;
	reg_id[30] = SPR_SPRG1;
	reg_id[31] = SPR_SPRG2;
	reg_id[32] = SPR_SPRG3;
	reg_id[33] = SPR_PPR;

	ret = sbefifo_register_get(sctx,
				   pdbg_target_index(chiplet),
				   thread->id,
				   SBEFIFO_REGISTER_TYPE_SPR,
				   reg_id,
				   34,
				   &value);
	if (ret)
		return ret;

	regs->nia = value[0];
	regs->msr = value[1];
	regs->cfar = value[2];
	regs->lr = value[3];
	regs->ctr = value[4];
	regs->tar = value[5];
	regs->cr = (uint32_t)(value[6] & 0xffffffff);
	regs->xer = value[7];
	regs->lpcr = value[8];
	regs->ptcr = value[9];
	regs->lpidr = value[10];
	regs->pidr = value[11];
	regs->hfscr = value[12];
	regs->hdsisr = (uint32_t)(value[13] & 0xffffffff);
	regs->hdar = value[14];
	regs->hsrr0 = value[15];
	regs->hsrr1 = value[16];
	regs->hdec = value[17];
	regs->heir = (uint32_t)(value[18] & 0xffffffff);
	regs->hid = value[19];
	regs->hsprg0 = value[20];
	regs->hsprg1 = value[21];
	regs->fscr = value[22];
	regs->dsisr = (uint32_t)(value[23] & 0xffffffff);
	regs->dar = value[24];
	regs->srr0 = value[25];
	regs->srr1 = value[26];
	regs->dec = value[27];
	regs->tb = value[28];
	regs->sprg0 = value[29];
	regs->sprg1 = value[30];
	regs->sprg2 = value[31];
	regs->sprg3 = value[32];
	regs->ppr = value[33];

	free(value);

	return 0;
}

static int sbefifo_thread_get_reg(struct thread *thread, uint8_t reg_type, uint32_t reg_id, uint64_t *value)
{
	struct pdbg_target *chiplet = pdbg_target_require_parent("chiplet", &thread->target);
	struct pdbg_target *pib = pdbg_target_require_parent("pib", chiplet);
	struct sbefifo *sbefifo = pib_to_sbefifo(pib);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);
	uint64_t *v;
	int ret;

	/* This chip-op requires core-id as pervasive (chiplet) id */
	ret = sbefifo_register_get(sctx,
				   pdbg_target_index(chiplet),
				   thread->id,
				   reg_type,
				   &reg_id,
				   1,
				   &v);
	if (ret)
		return ret;

	*value = *v;
	free(v);

	return 0;
}

static int sbefifo_thread_put_reg(struct thread *thread, uint8_t reg_type, uint32_t reg_id, uint64_t value)
{
	struct pdbg_target *chiplet = pdbg_target_require_parent("chiplet", &thread->target);
	struct pdbg_target *pib = pdbg_target_require_parent("pib", chiplet);
	struct sbefifo *sbefifo = pib_to_sbefifo(pib);
	struct sbefifo_context *sctx = sbefifo->get_sbefifo_context(sbefifo);

	/* This chip-op requires core-id as pervasive (chiplet) id */
	return sbefifo_register_put(sctx,
				    pdbg_target_index(chiplet),
				    thread->id,
				    reg_type,
				    &reg_id,
				    1,
				    &value);
}

static int sbefifo_thread_getgpr(struct thread *thread, int gpr, uint64_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_GPR;
	uint32_t reg_id = gpr;

	return sbefifo_thread_get_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_putgpr(struct thread *thread, int gpr, uint64_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_GPR;
	uint32_t reg_id = gpr;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_getspr(struct thread *thread, int spr, uint64_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = spr;

	return sbefifo_thread_get_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_putspr(struct thread *thread, int spr, uint64_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = spr;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_getfpr(struct thread *thread, int fpr, uint64_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_FPR;
	uint32_t reg_id = fpr;

	return sbefifo_thread_get_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_putfpr(struct thread *thread, int fpr, uint64_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_FPR;
	uint32_t reg_id = fpr;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_getmsr(struct thread *thread, uint64_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_MSR;

	return sbefifo_thread_get_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_putmsr(struct thread *thread, uint64_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_MSR;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_getnia(struct thread *thread, uint64_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_NIA;

	return sbefifo_thread_get_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_putnia(struct thread *thread, uint64_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_NIA;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_getxer(struct thread *thread, uint64_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_XER;

	return sbefifo_thread_get_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_putxer(struct thread *thread, uint64_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_XER;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, value);
}

static int sbefifo_thread_getcr(struct thread *thread, uint32_t *value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_CR;
	uint64_t val;
	int ret;

	ret = sbefifo_thread_get_reg(thread, reg_type, reg_id, &val);
	if (ret)
		return ret;

	*value = (uint32_t)(val & 0xffffffff);
	return 0;
}

static int sbefifo_thread_putcr(struct thread *thread, uint32_t value)
{
	uint8_t reg_type = SBEFIFO_REGISTER_TYPE_SPR;
	uint32_t reg_id = SPR_CR;
	uint64_t val = value;;

	return sbefifo_thread_put_reg(thread, reg_type, reg_id, val);
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
	/*
	 * FIXME: Need to add reference counting for sbefifo context, so it is
	 * not freed till every last hwunit using sbefifo driver has been
	 * released.
	 */
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
	.mpipl_get_ti_info = sbefifo_op_mpipl_get_ti_info,
};
DECLARE_HW_UNIT(sbefifo_chipop);

static struct pib sbefifo_pib = {
	.target = {
		.name = "SBE FIFO Chip-op based PIB",
		.compatible = "ibm,sbefifo-pib",
		.class = "pib",
	},
	.read = sbefifo_pib_read,
	.write = sbefifo_pib_write,
	.thread_start_all = sbefifo_pib_thread_start,
	.thread_stop_all = sbefifo_pib_thread_stop,
	.thread_step_all = sbefifo_pib_thread_step,
	.thread_sreset_all = sbefifo_pib_thread_sreset,
	.fd = -1,
};
DECLARE_HW_UNIT(sbefifo_pib);

static struct thread sbefifo_thread = {
	.target = {
		.name = "SBE FFIO Chip-op based Thread",
		.compatible = "ibm,power-thread",
		.class = "thread",
		.probe = sbefifo_thread_probe,
		.release = sbefifo_thread_release,
	},
	.start = sbefifo_thread_start,
	.stop = sbefifo_thread_stop,
	.step = sbefifo_thread_step,
	.sreset = sbefifo_thread_sreset,
	.getregs = sbefifo_thread_getregs,
	.getgpr = sbefifo_thread_getgpr,
	.putgpr = sbefifo_thread_putgpr,
	.getspr = sbefifo_thread_getspr,
	.putspr = sbefifo_thread_putspr,
	.getfpr = sbefifo_thread_getfpr,
	.putfpr = sbefifo_thread_putfpr,
	.getmsr = sbefifo_thread_getmsr,
	.putmsr = sbefifo_thread_putmsr,
	.getnia = sbefifo_thread_getnia,
	.putnia = sbefifo_thread_putnia,
	.getxer = sbefifo_thread_getxer,
	.putxer = sbefifo_thread_putxer,
	.getcr = sbefifo_thread_getcr,
	.putcr = sbefifo_thread_putcr,
};
DECLARE_HW_UNIT(sbefifo_thread);

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
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_pib_hw_unit);
	pdbg_hwunit_register(PDBG_BACKEND_SBEFIFO, &sbefifo_thread_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_mem_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &sbefifo_pba_hw_unit);
}
