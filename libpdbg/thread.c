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

#include "libpdbg.h"
#include "hwunit.h"
#include "debug.h"

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
	thread = target_to_thread(target);
	return thread->step(thread, count);
}

int thread_start(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->start(thread);
}

int thread_stop(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->stop(thread);
}

int thread_sreset(struct pdbg_target *target)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);
	return thread->sreset(thread);
}

int thread_step_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

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
	}

	return rc;
}

int thread_start_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

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
	}

	return rc;
}

int thread_stop_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

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
	}

	return rc;
}

int thread_sreset_all(void)
{
	struct pdbg_target *target, *thread;
	int rc = 0, count = 0;

	pdbg_for_each_class_target("pib", target) {
		struct pib *pib = target_to_pib(target);

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
	}

	return rc;
}

int thread_getmem(struct pdbg_target *target, uint64_t addr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	if (thread->getmem)
		return thread->getmem(thread, addr, value);

	PR_ERROR("Not implemented on the backend\n");
	return -1;
}

int thread_getregs(struct pdbg_target *target, struct thread_regs *regs)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	if (thread->getregs)
		return thread->getregs(thread, regs);

	PR_ERROR("Not implemented on the backend\n");
	return -1;
}

int thread_getgpr(struct pdbg_target *target, int gpr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->getgpr(thread, gpr, value);
}

int thread_putgpr(struct pdbg_target *target, int gpr, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->putgpr(thread, gpr, value);
}

int thread_getspr(struct pdbg_target *target, int spr, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->getspr(thread, spr, value);
}

int thread_putspr(struct pdbg_target *target, int spr, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->putspr(thread, spr, value);
}

int thread_getmsr(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->getmsr(thread, value);
}

int thread_putmsr(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->putmsr(thread, value);
}

int thread_getnia(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->getnia(thread, value);
}

int thread_putnia(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->putnia(thread, value);
}

int thread_getxer(struct pdbg_target *target, uint64_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->getxer(thread, value);
}

int thread_putxer(struct pdbg_target *target, uint64_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->putxer(thread, value);
}

int thread_getcr(struct pdbg_target *target, uint32_t *value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->getcr(thread, value);
}

int thread_putcr(struct pdbg_target *target, uint32_t value)
{
	struct thread *thread;

	assert(pdbg_target_is_class(target, "thread"));
	thread = target_to_thread(target);

	return thread->putcr(thread, value);
}
