/* Copyright 2020 IBM Corp.
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
#include <inttypes.h>

#include "libpdbg.h"
#include "hwunit.h"
#include "debug.h"
#include "sprs.h"

struct thread_state thread_status(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->status;
}

/*
 * Single step the thread count instructions.
 */
int thread_step(struct pdbg_target *target, int count)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->step) {
		PR_ERROR("step() not implemented for the target\n");
		return -1;
	}

	return thread->step(thread, count);
}

int thread_start(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->start) {
		PR_ERROR("start() not implemented for the target\n");
		return -1;
	}

	return thread->start(thread);
}

int thread_stop(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->stop) {
		PR_ERROR("stop() not implemented for the target\n");
		return -1;
	}

	return thread->stop(thread);
}

int thread_sreset(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->sreset) {
		PR_ERROR("sreset() not implemented for the target\n");
		return -1;
	}

	return thread->sreset(thread);
}

int thread_step_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!pib->thread_step_all)
			break;

		rc |= pib->thread_step_all(pib, 1);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_step(thread, 1);
		count++;
	}

	if (count > 0)
		return rc;

	return -1;
}

int thread_start_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!pib->thread_start_all)
			break;

		rc |= pib->thread_start_all(pib);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_start(thread);
		count++;
	}

	if (count > 0)
		return rc;

	return -1;
}

int thread_stop_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!pib->thread_stop_all)
			break;

		rc |= pib->thread_stop_all(pib);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_stop(thread);
		count++;
	}

	if (count > 0)
		return rc;

	return -1;
}

int thread_sreset_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		if (!pib->thread_sreset_all)
			break;

		rc |= pib->thread_sreset_all(pib);
		count++;
	}

	if (count > 0)
		return rc;

	pdbg_for_each_class_target("thread", thread) {
		if (pdbg_target_status(thread) != PDBG_TARGET_ENABLED)
			continue;

		rc |= thread_sreset(thread);
		count++;
	}

	if (count > 0)
		return rc;

	return -1;
}

int thread_getmem(struct pdbg_target *target, uint64_t addr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getmem) {
		PR_ERROR("getmem() not implemented for the target\n");
		return -1;
	}

	return thread->getmem(thread, addr, value);
}

void thread_print_regs(struct thread_regs *regs)
{
	int i;

	printf("NIA   : 0x%016" PRIx64 "\n", regs->nia);
	printf("CFAR  : 0x%016" PRIx64 "\n", regs->cfar);
	printf("MSR   : 0x%016" PRIx64 "\n", regs->msr);
	printf("LR    : 0x%016" PRIx64 "\n", regs->lr);
	printf("CTR   : 0x%016" PRIx64 "\n", regs->ctr);
	printf("TAR   : 0x%016" PRIx64 "\n", regs->tar);
	printf("CR    : 0x%08" PRIx32 "\n", regs->cr);
	printf("XER   : 0x%08" PRIx64 "\n", regs->xer);
	printf("GPRS  :\n");
	for (i = 0; i < 32; i++) {
		printf(" 0x%016" PRIx64 "", regs->gprs[i]);
		if (i % 4 == 3)
			printf("\n");
	}
	printf("LPCR  : 0x%016" PRIx64 "\n", regs->lpcr);
	printf("PTCR  : 0x%016" PRIx64 "\n", regs->ptcr);
	printf("LPIDR : 0x%016" PRIx64 "\n", regs->lpidr);
	printf("PIDR  : 0x%016" PRIx64 "\n", regs->pidr);
	printf("HFSCR : 0x%016" PRIx64 "\n", regs->hfscr);
	printf("HDSISR: 0x%08" PRIx32 "\n", regs->hdsisr);
	printf("HDAR  : 0x%016" PRIx64 "\n", regs->hdar);
	printf("HEIR  : 0x%016" PRIx32 "\n", regs->heir);
	printf("HID0  : 0x%016" PRIx64 "\n", regs->hid);
	printf("HSRR0 : 0x%016" PRIx64 "\n", regs->hsrr0);
	printf("HSRR1 : 0x%016" PRIx64 "\n", regs->hsrr1);
	printf("HDEC  : 0x%016" PRIx64 "\n", regs->hdec);
	printf("HSPRG0: 0x%016" PRIx64 "\n", regs->hsprg0);
	printf("HSPRG1: 0x%016" PRIx64 "\n", regs->hsprg1);
	printf("FSCR  : 0x%016" PRIx64 "\n", regs->fscr);
	printf("DSISR : 0x%08" PRIx32 "\n", regs->dsisr);
	printf("DAR   : 0x%016" PRIx64 "\n", regs->dar);
	printf("SRR0  : 0x%016" PRIx64 "\n", regs->srr0);
	printf("SRR1  : 0x%016" PRIx64 "\n", regs->srr1);
	printf("DEC   : 0x%016" PRIx64 "\n", regs->dec);
	printf("TB    : 0x%016" PRIx64 "\n", regs->tb);
	printf("SPRG0 : 0x%016" PRIx64 "\n", regs->sprg0);
	printf("SPRG1 : 0x%016" PRIx64 "\n", regs->sprg1);
	printf("SPRG2 : 0x%016" PRIx64 "\n", regs->sprg2);
	printf("SPRG3 : 0x%016" PRIx64 "\n", regs->sprg3);
	printf("PPR   : 0x%016" PRIx64 "\n", regs->ppr);
}

int thread_getregs(struct pdbg_target *target, struct thread_regs *regs)
{
	struct thread *thread;
	int err;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getregs) {
		PR_ERROR("getregs() not implemented for the target\n");
		return -1;
	}

	err = thread->getregs(thread, regs);
	if (!err)
		thread_print_regs(regs);

	return err;
}

int thread_getgpr(struct pdbg_target *target, int gpr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getgpr) {
		PR_ERROR("getgpr() not implemented for the target\n");
		return -1;
	}

	return thread->getgpr(thread, gpr, value);
}

int thread_putgpr(struct pdbg_target *target, int gpr, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->putgpr) {
		PR_ERROR("putgpr() not implemented for the target\n");
		return -1;
	}

	return thread->putgpr(thread, gpr, value);
}

int thread_getspr(struct pdbg_target *target, int spr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (spr == SPR_MSR) {
		if (thread->getmsr)
			return thread->getmsr(thread, value);
	} else if (spr == SPR_NIA) {
		if (thread->getnia)
			return thread->getnia(thread, value);
	} else if (spr == SPR_XER) {
		if (thread->getxer)
			return thread->getxer(thread, value);
	} else if (spr == SPR_CR) {
		if (thread->getcr) {
			uint32_t u32;
			int rc;

			rc  = thread->getcr(thread, &u32);
			if (rc == 0)
				*value = u32;

			return rc;
		}
	}

	if (!thread->getspr) {
		PR_ERROR("getspr() not implemented for the target\n");
		return -1;
	}

	return thread->getspr(thread, spr, value);
}

int thread_putspr(struct pdbg_target *target, int spr, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (spr == SPR_MSR) {
		if (thread->putmsr)
			return thread->putmsr(thread, value);
	} else if (spr == SPR_NIA) {
		if (thread->putnia)
			return thread->putnia(thread, value);
	} else if (spr == SPR_XER) {
		if (thread->putxer)
			return thread->putxer(thread, value);
	} else if (spr == SPR_CR) {
		if (thread->putcr)
			return thread->putcr(thread, (uint32_t)value);
	}

	if (!thread->putspr) {
		PR_ERROR("putspr() not implemented for the target\n");
		return -1;
	}

	return thread->putspr(thread, spr, value);
}

int thread_getfpr(struct pdbg_target *target, int fpr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getfpr) {
		PR_ERROR("getfpr() not implemented for the target\n");
		return -1;
	}

	return thread->getfpr(thread, fpr, value);
}

int thread_putfpr(struct pdbg_target *target, int fpr, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->putfpr) {
		PR_ERROR("putfpr() not implemented for the target\n");
		return -1;
	}

	return thread->putfpr(thread, fpr, value);
}

int thread_getmsr(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getmsr) {
		PR_ERROR("getmsr() not implemented for the target\n");
		return -1;
	}

	return thread->getmsr(thread, value);
}

int thread_putmsr(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->putmsr) {
		PR_ERROR("putmsr() not implemented for the target\n");
		return -1;
	}

	return thread->putmsr(thread, value);
}

int thread_getnia(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getnia) {
		PR_ERROR("getnia() not implemented for the target\n");
		return -1;
	}

	return thread->getnia(thread, value);
}

int thread_putnia(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->putnia) {
		PR_ERROR("putnia() not implemented for the target\n");
		return -1;
	}

	return thread->putnia(thread, value);
}

int thread_getxer(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getxer) {
		PR_ERROR("getxer() not implemented for the target\n");
		return -1;
	}

	return thread->getxer(thread, value);
}

int thread_putxer(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->putxer) {
		PR_ERROR("putxer() not implemented for the target\n");
		return -1;
	}

	return thread->putxer(thread, value);
}

int thread_getcr(struct pdbg_target *target, uint32_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	thread = target_to_thread(target);

	if (!thread->getcr) {
		PR_ERROR("getcr() not implemented for the target\n");
		return -1;
	}

	return thread->getcr(thread, value);
}

int thread_putcr(struct pdbg_target *target, uint32_t value)
{
	struct thread *thread;

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	assert(pdbg_target_is_class(target, "thread"));

	thread = target_to_thread(target);

	if (!thread->putcr) {
		PR_ERROR("putcr() not implemented for the target\n");
		return -1;
	}

	return thread->putcr(thread, value);
}
