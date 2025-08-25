/* Copyright 2021 IBM Corp.
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
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "target.h"
#include "hwunit.h"
#include "debug.h"
#include "libpdbg_sbe.h"

#define SBE_MSG_REG	0x2809
#define SBE_STATE_REG	0x2986

enum {
	SBE_MSG_STATE_UNKNOWN = 0x0,
	SBE_MSG_STATE_IPLING  = 0x1,
	SBE_MSG_STATE_ISTEP   = 0x2,
	SBE_MSG_STATE_MPIPL   = 0x3,
	SBE_MSG_STATE_RUNTIME = 0x4,
	SBE_MSG_STATE_DMT     = 0x5,
	SBE_MSG_STATE_DUMP    = 0x6,
	SBE_MSG_STATE_FAILURE = 0x7,
	SBE_MSG_STATE_QUIESCE = 0x8,

	SBE_MSG_STATE_INVALID = 0xF,
};

union sbe_msg_register {
	uint32_t reg;
	struct {
		uint32_t reserved2: 6;
		uint32_t minor_step: 6;
		uint32_t major_step: 8;
		uint32_t curr_state: 4;
		uint32_t prev_state: 4;
		uint32_t reserved1: 2;
		uint32_t async_ffdc: 1;
		uint32_t sbe_booted: 1;
	};
};

static struct chipop *pib_to_chipop(struct pdbg_target *pib)
{
	struct pdbg_target *chipop;
	uint32_t index;

	assert(pdbg_target_is_class(pib, "pib"));

	if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
		return NULL;

	index = pdbg_target_index(pib);

	pdbg_for_each_class_target("chipop", chipop) {
		if (pdbg_target_index(chipop) != index)
			continue;

		if (pdbg_target_probe(chipop) == PDBG_TARGET_ENABLED)
			return target_to_chipop(chipop);
	}

	return NULL;
}

int sbe_istep(struct pdbg_target *target, uint32_t major, uint32_t minor)
{
	struct chipop *chipop;
	int rc;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->istep) {
		PR_ERROR("istep() not implemented for the target\n");
		return -1;
	}

	rc = chipop->istep(chipop, major, minor);
	if (rc) {
		PR_ERROR("sbe istep() returned rc=%d\n", rc);
		return -1;
	}

	return 0;
}

int sbe_mpipl_enter(struct pdbg_target *target)
{
	struct chipop *chipop;
	int rc;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_enter) {
		PR_ERROR("mpipl_enter() not implemented for the target\n");
		return -1;
	}

	rc = chipop->mpipl_enter(chipop);
	if (rc) {
		PR_ERROR("sbe mpipl_enter() returned rc=%d\n", rc);
		return -1;
	}

	return 0;
}

int sbe_mpipl_continue(struct pdbg_target *target)
{
	struct chipop *chipop;
	int rc;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_continue) {
		PR_ERROR("mpipl_continue() not implemented for the target\n");
		return -1;
	}

	rc = chipop->mpipl_continue(chipop);
	if (rc) {
		PR_ERROR("sbe mpipl_continue() returned rc=%d\n", rc);
		return -1;
	}

	return 0;
}

int sbe_mpipl_get_ti_info(struct pdbg_target *target, uint8_t **data, uint32_t *data_len)
{
	struct chipop *chipop;
	int rc;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_get_ti_info) {
		PR_ERROR("mpipl_get_ti_info() not implemented for the target\n");
		return -1;
	}

	rc = chipop->mpipl_get_ti_info(chipop, data, data_len);
	if (rc) {
		PR_ERROR("sbe mpipl_get_ti_info() returned rc=%d\n", rc);
		return -1;
	}

	return 0;
}

int sbe_dump(struct pdbg_target *target, uint8_t type, uint8_t clock,
		uint8_t fa_collect, uint8_t **data, uint32_t *data_len)
{
	if(!is_ody_ocmb_chip(target)) {
		struct chipop *chipop;
		int rc;

		chipop = pib_to_chipop(target);
		if (!chipop)
			return -1;

		if (!chipop->dump) {
			PR_ERROR("dump() not implemented for the target\n");
			return -1;
		}

		rc = chipop->dump(chipop, type, clock, fa_collect, data, data_len);
		if (rc) {
			PR_ERROR("sbe dump() returned rc=%d\n", rc);
			return -1;
		}
	} else {
		struct chipop_ody *chipop;
		int rc;
		struct pdbg_target *co_target = get_ody_chipop_target(target);
		chipop = target_to_chipop_ody(co_target);
		if (!chipop)
			return -1;

		if (!chipop->dump) {
			PR_ERROR("dump() not implemented for the target\n");
			return -1;
		}
		rc = chipop->dump(chipop, type, clock, fa_collect, data, data_len);
		if (rc) {
			PR_ERROR("sbe dump() returned rc=%d\n", rc);
			return -1;
		}
	}
	return 0;
}


int sbe_operation(struct pdbg_target* target,
                  uint8_t* msg, uint32_t msg_len,
                  uint8_t** out, uint32_t* out_len)
{
	PR_ERROR("In sbe_operation");
    if (!is_ody_ocmb_chip(target)) {
        struct chipop* chipop = pib_to_chipop(target);
        if (!chipop || !chipop->operation) {
            PR_ERROR("operation() not implemented for the target\n");
            return -1;
        }
		PR_ERROR("In sbe_operation: calling chipop->operation");
        return chipop->operation(chipop, msg, msg_len, out, out_len);
    } else {
        struct chipop_ody* chipop;
        struct pdbg_target* co_target = get_ody_chipop_target(target);
        chipop = target_to_chipop_ody(co_target);
        if (!chipop || !chipop->operation) {
            PR_ERROR("operation() not implemented for the ODY target\n");
            return -1;
        }
        return chipop->operation(chipop, msg, msg_len, out, out_len);
    }
}

int sbe_set_chipop_timeout(struct pdbg_target* target, uint32_t timeout_ms)
{
    if (!is_ody_ocmb_chip(target)) {
	
        struct chipop* chipop = pib_to_chipop(target);
        if (!chipop || !chipop->set_fifo_timeout) {
            PR_ERROR("FIFO timeout not supported for the target\n");
            return -1;
        }
        return chipop->set_fifo_timeout(chipop, timeout_ms);
    } else {
        struct chipop_ody* chipop;
        struct pdbg_target* co_target = get_ody_chipop_target(target);
        chipop = target_to_chipop_ody(co_target);
        if (!chipop || !chipop->set_fifo_timeout) {
            PR_ERROR("FIFO timeout not supported for the ODY target\n");
            return -1;
        }
        return chipop->set_fifo_timeout((struct chipop_ody*)chipop, timeout_ms);
    }
}

int sbe_ffdc_get(struct pdbg_target *target, uint32_t *status, uint8_t **ffdc,
				uint32_t *ffdc_len)
{
	if(!is_ody_ocmb_chip(target)) {
		struct chipop *chipop;
		const uint8_t *data = NULL;
		uint32_t len = 0;

		chipop = pib_to_chipop(target);
		if (!chipop)
			return -1;

		if (!chipop->ffdc_get) {
			PR_ERROR("ffdc_get() not implemented for the target\n");
			return -1;
		}

		*status = chipop->ffdc_get(chipop, &data, &len);
		if (data && len > 0) {
			*ffdc = malloc(len);
			assert(*ffdc);
			memcpy(*ffdc, data, len);
			*ffdc_len = len;
		} else {
			*ffdc = NULL;
			*ffdc_len = 0;
		}
	} else {
		struct chipop_ody *chipop;
		const uint8_t *data = NULL;
		uint32_t len = 0;

		struct pdbg_target *co_target = get_ody_chipop_target(target);
		chipop = target_to_chipop_ody(co_target);
		if (!chipop) {
			PR_ERROR("chipop target not found for ody ocmb chip\n");
			return -1;
		}
		if (!chipop->ffdc_get) {
			PR_ERROR("ffdc_get() not implemented for the target\n");
			return -1;
		}
		struct pdbg_target *fsi = get_ody_fsi_target(target);
		if (!fsi) {
			PR_ERROR("fsi target not found for ody ocmb chip\n");
			return -1;
		}
		*status = chipop->ffdc_get(chipop, fsi, &data, &len);
		if (data && len > 0) {
			*ffdc = malloc(len);
			assert(*ffdc);
			memcpy(*ffdc, data, len);
			*ffdc_len = len;
		} else {
			*ffdc = NULL;
			*ffdc_len = 0;
		}
	}
	return 0;
}

static int sbe_read_msg_register(struct pdbg_target *pib, uint32_t *value)
{
	struct pdbg_target *fsi = pdbg_target_parent_virtual("fsi", pib);
	int rc;

	assert(pdbg_target_is_class(pib, "pib"));
	assert(fsi);

	if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
		return -1;

	rc = fsi_read(fsi, SBE_MSG_REG, value);
	if (rc) {
		PR_NOTICE("Failed to read sbe mailbox register\n");
		return rc;
	}

	return 0;
}

static int sbe_read_state_register(struct pdbg_target *pib, uint32_t *value)
{
	struct pdbg_target *fsi = pdbg_target_parent_virtual("fsi", pib);
	int rc;

	assert(pdbg_target_is_class(pib, "pib"));
	assert(fsi);

	if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
		return -1;

	rc = fsi_read(fsi, SBE_STATE_REG, value);
	if (rc) {
		PR_NOTICE("Failed to read sbe state register\n");
		return rc;
	}

	return 0;
}

static int sbe_ody_read_msg_register(struct pdbg_target *fsi, uint32_t *value)
{
	int rc;

	assert(fsi);
	assert(pdbg_target_is_class(fsi, "fsi-ody"));

	if (pdbg_target_status(fsi) != PDBG_TARGET_ENABLED)
		return -1;

	rc = fsi_ody_read(fsi, SBE_MSG_REG, value);
	if (rc) {
		PR_NOTICE("Failed to read ody sbe mailbox register\n");
		return rc;
	}

	return 0;
}

static int sbe_ody_read_state_register(struct pdbg_target *fsi, uint32_t *value)
{
	int rc;

	assert(fsi);
	assert(pdbg_target_is_class(fsi, "fsi-ody"));

	if (pdbg_target_status(fsi) != PDBG_TARGET_ENABLED)
		return -1;

	rc = fsi_ody_read(fsi, SBE_STATE_REG, value);
	if (rc) {
		PR_NOTICE("Failed to read ody sbe state register\n");
		return rc;
	}
	return 0;
}

static int sbe_write_state_register(struct pdbg_target *pib, uint32_t value)
{
	struct pdbg_target *fsi = pdbg_target_parent_virtual("fsi", pib);
	int rc;

	assert(pdbg_target_is_class(pib, "pib"));
	assert(fsi);

	if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
		return -1;

	rc = fsi_write(fsi, SBE_STATE_REG, value);
	if (rc) {
		PR_NOTICE("Failed to write sbe state register\n");
		return rc;
	}

	return 0;
}

static int sbe_ody_write_state_register(struct pdbg_target *fsi, uint32_t value)
{
	int rc;

	assert(fsi);
	assert(pdbg_target_is_class(fsi, "fsi-ody"));

	if (pdbg_target_status(fsi) != PDBG_TARGET_ENABLED)
		return -1;

	rc = fsi_ody_write(fsi, SBE_STATE_REG, value);
	if (rc) {
		PR_NOTICE("Failed to write ody sbe state register\n");
		return rc;
	}

	return 0;
}

int sbe_get_state(struct pdbg_target *pib, enum sbe_state *state)
{
	union sbe_msg_register msg;
	uint32_t value;
	int rc;

	rc = sbe_read_state_register(pib, &value);
	if (rc)
		return -1;

	if (value == SBE_STATE_CHECK_CFAM) {
		rc = sbe_read_msg_register(pib, &msg.reg);
		if (rc)
			return -1;

		*state = msg.sbe_booted ? SBE_STATE_BOOTED : SBE_STATE_CHECK_CFAM;
	} else {
		*state = value;
	}

	return 0;
}

int sbe_ody_get_state(struct pdbg_target *fsi, enum sbe_state *state)
{
	union sbe_msg_register msg;
	uint32_t value;
	int rc;

	rc = sbe_ody_read_state_register(fsi, &value);
	if (rc)
		return -1;
	if ((value == SBE_STATE_CHECK_CFAM) || (value == SBE_MSG_STATE_UNKNOWN)) {
		rc = sbe_ody_read_msg_register(fsi, &msg.reg);
		if (rc)
			return -1;
		*state = msg.sbe_booted ? SBE_STATE_BOOTED : SBE_STATE_CHECK_CFAM;
	} else {
		*state = value;
	}

	return 0;
}

int sbe_set_state(struct pdbg_target *pib, enum sbe_state state)
{
	uint32_t value = state;
	int rc;

	rc = sbe_write_state_register(pib, value);
	if (rc)
		return -1;

	return 0;
}

int sbe_ody_set_state(struct pdbg_target *fsi, enum sbe_state state)
{
	uint32_t value = state;
	int rc;

	rc = sbe_ody_write_state_register(fsi, value);
	if (rc)
		return -1;

	return 0;
}

int sbe_is_ipl_done(struct pdbg_target *pib, bool *done)
{
	union sbe_msg_register msg;
	int rc;

	rc = sbe_read_msg_register(pib, &msg.reg);
	if (rc)
		return -1;

	*done = false;
	if ((msg.curr_state == SBE_MSG_STATE_RUNTIME) ||
	    (msg.prev_state == SBE_MSG_STATE_RUNTIME)) {
		*done = true;
	}

	return 0;
}

int sbe_lpc_timeout(struct pdbg_target *target, uint32_t *timeout_flag)
{
	struct chipop *chipop;
	int rc;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->lpc_timeout) {
		PR_ERROR("lpc_timeout() not implemented for the target\n");
		return -1;
	}

	rc = chipop->lpc_timeout(chipop, timeout_flag);
	if (rc) {
		PR_ERROR("sbe lpc_timeout() returned rc=%d\n", rc);
		return -1;
	}

	return 0;
}
