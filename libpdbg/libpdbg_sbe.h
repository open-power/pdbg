#ifndef __LIBPDBG_SBE_H
#define __LIBPDBG_SBE_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "libpdbg.h"

enum sbe_state {
	SBE_STATE_NOT_USABLE  = 0x00000000,
	SBE_STATE_BOOTED      = 0x00000001,
	SBE_STATE_CHECK_CFAM  = 0x00000002,
	SBE_STATE_DEBUG_MODE  = 0x00000003,
	SBE_STATE_FAILED      = 0x00000004,

	SBE_STATE_INVALID     = 0x0000000F,
};

/**
 * @brief Execute IPL istep using SBE
 *
 * @param[in]  target pib target to operate on
 * @param[in]  major istep major number
 * @param[in]  minor istep minor number
 *
 * @return 0 on success, -1 on failure
 */
int sbe_istep(struct pdbg_target *target, uint32_t major, uint32_t minor);

/**
 * @brief Get FFDC data if error is generated
 *
 * @param[in]  target pib target to operate on
 * @param[out] status The status word
 * @param[out] ffdc pointer to output buffer to store ffdc data
 * @param[out] ffdc_len the sizeof the ffdc data returned
 *
 * @return 0 on success, -1 on failure
 *
 * The ffdc data must be freed by caller.  It is allocated using malloc and
 * must be freed using free().
 */
int sbe_ffdc_get(struct pdbg_target *target, uint32_t *status, uint8_t **ffdc, uint32_t *ffdc_len);

/**
 * @brief Execute enter mpipl on the pib
 *
 * @param[in] target pib target to operate on
 *
 * @return 0 on success, -1 on failure
 */
int sbe_mpipl_enter(struct pdbg_target *target);

/**
 * @brief Execute continue mpipl on the pib
 *
 * @param[in] target pib target to operate on
 *
 * @return 0 on success, -1 on failure
 */
int sbe_mpipl_continue(struct pdbg_target *target);

/**
 * @brief Get ti info
 *
 * @param[in] target pib target to operate on
 * @param[out] data TI information
 * @param[out] data_len length of the data
 *
 * @return 0 on success, -1 on failure
 */
int sbe_mpipl_get_ti_info(struct pdbg_target *target, uint8_t **data, uint32_t *data_len);

/**
 * @brief Get sbe dump
 *
 * The dump data must be freed by caller.  It is allocated using malloc() and
 * must be freed using free().
 *
 * @param[in] target pib target to operate on
 * @param[in] type Type of dump
 * @param[in] clock Clock on or off
 * @param[in] fa_collect Fast Array collection (0 off, 1 on)
 * @param[out] data Dump information
 * @param[out] data_len length of the data
 *
 * @return 0 on success, -1 on failure
 */
int sbe_dump(struct pdbg_target *target, uint8_t type, uint8_t clock, uint8_t fa_collect, uint8_t **data, uint32_t *data_len);

/**
 * @brief Get sbe state
 *
 * Get the current state of SBE
 *
 * @param[in] target pib target to operate on
 * @param[out] state sbe state
 *
 * @return 0 on success, -1 on failure
 */
int sbe_get_state(struct pdbg_target *target, enum sbe_state *state);

/**
 * @brief Get odyssey sbe state
 *
 * Get the current state of odyssey SBE
 *
 * @param[in] target fsi target to operate on
 * @param[out] state sbe state
 *
 * @return 0 on success, -1 on failure
 */
int sbe_ody_get_state(struct pdbg_target *fsi, enum sbe_state *state);

/**
 * @brief Set sbe state
 *
 * Set the current state of SBE
 *
 * @param[in] target pib target to operate on
 * @param[in] state sbe state
 *
 * @return 0 on success, -1 on failure
 */
int sbe_set_state(struct pdbg_target *target, enum sbe_state state);

/**
 * @brief Set odyssey sbe state
 *
 * Set the current state of odyssey SBE
 *
 * @param[in] fsi fsi target to operate on
 * @param[in] state odyssey sbe state
 *
 * @return 0 on success, -1 on failure
 */
int sbe_ody_set_state(struct pdbg_target *fsi, enum sbe_state state);

/**
 * @brief Check if IPL boot is complete
 *
 * @param[in] target pib target to operate on
 * @param[out] done true if IPL is completed, false otherwise
 *
 * @return 0 on success, -1 on failure
 */
int sbe_is_ipl_done(struct pdbg_target *target, bool *done);

/**
 * @brief Get the LPC timeout flag
 *
 * @param[in] target pib target to operate on
 * @param[out] timeout_flag 0 if no timeout ocurred, 1 if LPC timed out
 *
 * @return 0 on success, -1 on failure
 */
int sbe_lpc_timeout(struct pdbg_target *target, uint32_t *timeout_flag);


#ifdef __cplusplus
}
#endif

#endif /* __LIBPDBG_SBE_H */

