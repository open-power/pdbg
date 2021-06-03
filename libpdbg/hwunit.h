/* Copyright 2019 IBM Corp.
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
#ifndef __HWUNIT_H
#define __HWUNIT_H

#include <stdint.h>

#include "target.h"

/* This works and should be safe because struct pdbg_target is guaranteed to be
 * the first member of the specialised type (see the DECLARE_HW_UNIT definition
 * below). I'm not sure how sane it is though. Probably not very but it does
 * remove a bunch of tedious container_of() typing */
#define translate_cast(x) (uint64_t (*)(struct pdbg_target *, uint64_t)) (x)

struct hw_unit_info {
	void *hw_unit;
	size_t size;
};

void pdbg_hwunit_register(enum pdbg_backend backend, const struct hw_unit_info *hw_unit);
const struct hw_unit_info *pdbg_hwunit_find_compatible(const char *compat_list,
						       uint32_t len);

/*
 * If this macro fails to compile for you, you've probably not
 * declared the struct pdbg_target as the first member of the
 * container struct. Not doing so will break other assumptions.
 */
#define DECLARE_HW_UNIT(name)						\
	__attribute__((unused)) static inline void name ##_hw_unit_check(void) {	\
		((void)sizeof(char[1 - 2 * (container_off(typeof(name), target) != 0)])); \
	}								\
	const struct hw_unit_info __used name ##_hw_unit =              \
	{ .hw_unit = &name, .size = sizeof(name) };

struct proc {
	struct pdbg_target target;
};

struct htm {
	struct pdbg_target target;
	int (*start)(struct htm *);
	int (*stop)(struct htm *);
	int (*status)(struct htm *);
	int (*dump)(struct htm *, char *);
	int (*record)(struct htm *, char *);
};
#define target_to_htm(x) container_of(x, struct htm, target)

struct mem {
	struct pdbg_target target;
	int (*getmem)(struct mem *, uint64_t, uint64_t *, int, uint8_t);
	int (*putmem)(struct mem *, uint64_t, uint64_t, int, int, uint8_t);
	int (*read)(struct mem *, uint64_t, uint8_t *, uint64_t, uint8_t, bool);
	int (*write)(struct mem *, uint64_t, uint8_t *, uint64_t, uint8_t, bool);
};
#define target_to_mem(x) container_of(x, struct mem, target)

struct chipop {
	struct pdbg_target target;
	uint32_t (*ffdc_get)(struct chipop *, const uint8_t **, uint32_t *);
	int (*istep)(struct chipop *, uint32_t major, uint32_t minor);
	int (*mpipl_enter)(struct chipop *);
	int (*mpipl_continue)(struct chipop *);
	int (*mpipl_get_ti_info)(struct chipop *, uint8_t **, uint32_t *);
	int (*dump)(struct chipop *, uint8_t, uint8_t, uint8_t, uint8_t **, uint32_t *);
};
#define target_to_chipop(x) container_of(x, struct chipop, target)

struct sbefifo {
	struct pdbg_target target;
	struct sbefifo_context *sf_ctx;
	struct sbefifo_context * (*get_sbefifo_context)(struct sbefifo *);
};
#define target_to_sbefifo(x) container_of(x, struct sbefifo, target)

struct pib {
	struct pdbg_target target;
	int (*read)(struct pib *, uint64_t, uint64_t *);
	int (*write)(struct pib *, uint64_t, uint64_t);
	int (*thread_start_all)(struct pib *);
	int (*thread_stop_all)(struct pib *);
	int (*thread_step_all)(struct pib *, int);
	int (*thread_sreset_all)(struct pib *);
	void *priv;
	int fd;
};
#define target_to_pib(x) container_of(x, struct pib, target)

struct opb {
	struct pdbg_target target;
	int (*read)(struct opb *, uint32_t, uint32_t *);
	int (*write)(struct opb *, uint32_t, uint32_t);
};
#define target_to_opb(x) container_of(x, struct opb, target)

struct fsi {
	struct pdbg_target target;
	int (*read)(struct fsi *, uint32_t, uint32_t *);
	int (*write)(struct fsi *, uint32_t, uint32_t);
	enum chip_type chip_type;
	int fd;
};
#define target_to_fsi(x) container_of(x, struct fsi, target)

struct core {
	struct pdbg_target target;
	bool release_spwkup;
};
#define target_to_core(x) container_of(x, struct core, target)

struct thread {
	struct pdbg_target target;
	struct thread_state status;
	int id;
	struct thread_state (*state)(struct thread *);
	int (*step)(struct thread *, int);
	int (*start)(struct thread *);
	int (*stop)(struct thread *);
	int (*sreset)(struct thread *);

	bool ram_did_quiesce; /* was the thread quiesced by ram mode */

	/* ram_setup() should be called prior to using ram_instruction() to
	 * actually ram the instruction and return the result. ram_destroy()
	 * should be called at completion to clean-up. */
	bool ram_is_setup;
	int (*ram_setup)(struct thread *);
	int (*ram_instruction)(struct thread *, uint64_t opcode, uint64_t *scratch);
	int (*ram_destroy)(struct thread *);
	int (*enable_attn)(struct thread *);

	int (*getmem)(struct thread *, uint64_t, uint64_t *);
	int (*getregs)(struct thread *, struct thread_regs *regs);

	int (*getgpr)(struct thread *, int, uint64_t *);
	int (*putgpr)(struct thread *, int, uint64_t);

	int (*getspr)(struct thread *, int, uint64_t *);
	int (*putspr)(struct thread *, int, uint64_t);

	int (*getfpr)(struct thread *, int, uint64_t *);
	int (*putfpr)(struct thread *, int, uint64_t);

	int (*getmsr)(struct thread *, uint64_t *);
	int (*putmsr)(struct thread *, uint64_t);

	int (*getnia)(struct thread *, uint64_t *);
	int (*putnia)(struct thread *, uint64_t);

	int (*getxer)(struct thread *, uint64_t *);
	int (*putxer)(struct thread *, uint64_t);

	int (*getcr)(struct thread *, uint32_t *);
	int (*putcr)(struct thread *, uint32_t);
};
#define target_to_thread(x) container_of(x, struct thread, target)

/* Place holder for chiplets which we just want translation for */
struct chiplet {
        struct pdbg_target target;
	int (*getring)(struct chiplet *, uint64_t, int64_t, uint32_t[]);
};
#define target_to_chiplet(x) container_of(x, struct chiplet, target)

struct xbus {
	struct pdbg_target target;
	uint32_t ring_id;
};
#define target_to_xbus(x) container_of(x, struct xbus, target)

struct ex {
	struct pdbg_target target;
};
#define target_to_ex(x) container_of(x, struct ex, target)

struct mba {
	struct pdbg_target target;
};
#define target_to_mba(x) container_of(x, struct mba, target)

struct mcs {
	struct pdbg_target target;
};
#define target_to_mcs(x) container_of(x, struct mcs, target)

struct abus {
	struct pdbg_target target;
};
#define target_to_abus(x) container_of(x, struct abus, target)

struct l4 {
	struct pdbg_target target;
};
#define target_to_l4(x) container_of(x, struct l4, target)

struct eq {
	struct pdbg_target target;
};
#define target_to_eq(x) container_of(x, struct eq, target)

struct mca {
	struct pdbg_target target;
};
#define target_to_mca(x) container_of(x, struct mca, target)

struct mcbist {
	struct pdbg_target target;
};
#define target_to_mcbist(x) container_of(x, struct mcbist, target)

struct mi {
	struct pdbg_target target;
};
#define target_to_mi(x) container_of(x, struct mi, target)

struct dmi {
	struct pdbg_target target;
};
#define target_to_dmi(x) container_of(x, struct dmi, target)

struct obus {
	struct pdbg_target target;
};
#define target_to_obus(x) container_of(x, struct obus, target)

struct obus_brick {
	struct pdbg_target target;
};
#define target_to_obus_brick(x) container_of(x, struct obus_brick, target)

struct sbe {
	struct pdbg_target target;
};
#define target_to_sbe(x) container_of(x, struct sbe, target)

struct ppe {
	struct pdbg_target target;
};
#define target_to_ppe(x) container_of(x, struct ppe, target)

struct perv {
	struct pdbg_target target;
};
#define target_to_perv(x) container_of(x, struct perv, target)

struct pec {
	struct pdbg_target target;
};
#define target_to_pec(x) container_of(x, struct pec, target)

struct phb {
	struct pdbg_target target;
};
#define target_to_phb(x) container_of(x, struct phb, target)

struct mc {
	struct pdbg_target target;
};
#define target_to_mc(x) container_of(x, struct mc, target)

struct mem_port {
	struct pdbg_target target;
};
#define target_to_mem_port(x) container_of(x, struct mem_port, target)

struct nmmu {
	struct pdbg_target target;
};
#define target_to_nmmu(x) container_of(x, struct nmmu, target)

struct pau {
	struct pdbg_target target;
};
#define target_to_pau(x) container_of(x, struct pau, target)

struct iohs {
	struct pdbg_target target;
};
#define target_to_iohs(x) container_of(x, struct iohs, target)

struct fc {
	struct pdbg_target target;
};
#define target_to_fc(x) container_of(x, struct fc, target)

struct pauc {
	struct pdbg_target target;
};
#define target_to_pauc(x) container_of(x, struct pauc, target)

struct capp {
	struct pdbg_target target;
};
#define target_to_capp(x) container_of(x, struct capp, target)

struct omi {
	struct pdbg_target target;
};
#define target_to_omi(x) container_of(x, struct omi, target)

struct omic {
	struct pdbg_target target;
};
#define target_to_omic(x) container_of(x, struct omic, target)

struct mcc {
	struct pdbg_target target;
};
#define target_to_mcc(x) container_of(x, struct mcc, target)

struct ocmb {
	struct pdbg_target target;
	int (*getscom)(struct ocmb *, uint64_t, uint64_t *);
	int (*putscom)(struct ocmb *, uint64_t, uint64_t);
};
#define target_to_ocmb(x) container_of(x, struct ocmb, target);

struct smpgroup {
	struct pdbg_target target;
};
#define target_to_smpgroup(x) container_of(x, struct smpgroup, target)

struct dimm {
    struct pdbg_target target;
};
#define target_to_dimm(x) container_of(x, struct dimm, target)

#endif /* __HWUNIT_H */
