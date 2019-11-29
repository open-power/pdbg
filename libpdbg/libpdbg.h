#ifndef __LIBPDBG_H
#define __LIBPDBG_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pdbg_target;
struct pdbg_target_class;

/* loops/iterators */
struct pdbg_target *__pdbg_next_compatible_node(struct pdbg_target *root,
                                                struct pdbg_target *prev,
                                                const char *compat);
struct pdbg_target *__pdbg_next_target(const char *klass, struct pdbg_target *parent, struct pdbg_target *last, bool system);
struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last, bool system);

/*
 * Each target has a status associated with it. This is what each status means:
 *
 * enabled     - the target exists and has been probed, will be released by
 *               the release call.
 *
 * disabled    - the target has not been probed and will never be probed. Target
 *               selection code may use this to prevent probing of certain
 *               targets if it knows they are unnecessary.
 *
 * nonexistant - the target has been probed but did not exist. It will never be
 *               reprobed.
 *
 * unknown     - the target has not been probed but will be probed if required.
 *
 * mustexist   - the target has not been probed but an error will be reported if
 *               it does not exist as it is required for correct operation.
 *               Selection code may set this.
 *
 * released    - the target was enabled and has now been released.
 *
 * Initially these properties are read from the device tree. This allows the
 * client application to select which targets it does not care about to avoid
 * unneccessary probing by marking them disabled. If no status property exists
 * it defaults to "unknown".
 */
enum pdbg_target_status {PDBG_TARGET_UNKNOWN = 0, PDBG_TARGET_ENABLED,
			 PDBG_TARGET_DISABLED, PDBG_TARGET_MUSTEXIST,
			 PDBG_TARGET_NONEXISTENT, PDBG_TARGET_RELEASED};

enum pdbg_backend { PDBG_DEFAULT_BACKEND = 0, PDBG_BACKEND_FSI, PDBG_BACKEND_I2C,
		    PDBG_BACKEND_KERNEL, PDBG_BACKEND_FAKE, PDBG_BACKEND_HOST,
		    PDBG_BACKEND_CRONUS };

#define pdbg_for_each_compatible(parent, target, compat)		\
        for (target = NULL;                                             \
             (target = __pdbg_next_compatible_node(parent, target, compat)) != NULL;)

#define pdbg_for_each_target(class, parent, target)			\
	for (target = __pdbg_next_target(class, parent, NULL, true);	\
	     target;							\
	     target = __pdbg_next_target(class, parent, target, true))

#define pdbg_for_each_class_target(class, target)		\
	for (target = __pdbg_next_target(class, NULL, NULL, true);	\
	     target;						\
	     target = __pdbg_next_target(class, NULL, target, true))

#define pdbg_for_each_child_target(parent, target)	      \
	for (target = __pdbg_next_child_target(parent, NULL, true); \
	     target;					      \
	     target = __pdbg_next_child_target(parent, target, true))

/* Return the first parent target of the given class, or NULL if the given
 * target does not have a parent of the given class. */
struct pdbg_target *pdbg_target_parent(const char *klass, struct pdbg_target *target);
struct pdbg_target *pdbg_target_parent_virtual(const char *klass, struct pdbg_target *target);

/* Same as above but instead of returning NULL causes an assert failure. */
struct pdbg_target *pdbg_target_require_parent(const char *klass, struct pdbg_target *target);

/* Set the given property. Will automatically add one if one doesn't exist */
void pdbg_target_set_property(struct pdbg_target *target, const char *name, const void *val, size_t size);

/* Get the given property and return the size */
void *pdbg_target_property(struct pdbg_target *target, const char *name, size_t *size);
int pdbg_target_u32_property(struct pdbg_target *target, const char *name, uint32_t *val);
int pdbg_target_u32_index(struct pdbg_target *target, const char *name, int index, uint32_t *val);
uint64_t pdbg_target_address(struct pdbg_target *target, uint64_t *size);

/* Old deprecated for names for the above. Do not use for new projects
 * as these will be removed at some future point. */
#define pdbg_set_target_property(target, name, val, size)	\
	pdbg_target_set_property(target, name, val, size)
#define pdbg_get_target_property(target, name, size) \
	pdbg_target_property(target, name, size)
#define pdbg_get_address(target, index, size) \
	(index == 0 ? pdbg_target_address(target, size) : assert(0))

/* Misc. */
bool pdbg_targets_init(void *fdt);
void pdbg_target_probe_all(struct pdbg_target *parent);
enum pdbg_target_status pdbg_target_probe(struct pdbg_target *target);
void pdbg_target_release(struct pdbg_target *target);
enum pdbg_target_status pdbg_target_status(struct pdbg_target *target);
void pdbg_target_status_set(struct pdbg_target *target, enum pdbg_target_status status);
bool pdbg_set_backend(enum pdbg_backend backend, const char *backend_option);
uint32_t pdbg_target_index(struct pdbg_target *target);
char *pdbg_target_path(struct pdbg_target *target);
struct pdbg_target *pdbg_target_from_path(struct pdbg_target *target, const char *path);
uint32_t pdbg_parent_index(struct pdbg_target *target, char *klass);
const char *pdbg_target_class_name(struct pdbg_target *target);
const char *pdbg_target_name(struct pdbg_target *target);
const char *pdbg_target_dn_name(struct pdbg_target *target);
void *pdbg_target_priv(struct pdbg_target *target);
void pdbg_target_priv_set(struct pdbg_target *target, void *priv);
struct pdbg_target *pdbg_target_root(void);
bool pdbg_target_compatible(struct pdbg_target *target, const char *compatible);

const char *kernel_get_fsi_path(void);

/* Translate an address offset for a target to absolute address in address
 * space of a "base" target.  */
struct pdbg_target *pdbg_address_absolute(struct pdbg_target *target, uint64_t *addr);

/* Procedures */
int fsi_read(struct pdbg_target *target, uint32_t addr, uint32_t *val);
int fsi_write(struct pdbg_target *target, uint32_t addr, uint32_t val);
int fsi_write_mask(struct pdbg_target *target, uint32_t addr, uint32_t val, uint32_t mask);

int pib_read(struct pdbg_target *target, uint64_t addr, uint64_t *val);
int pib_write(struct pdbg_target *target, uint64_t addr, uint64_t val);
int pib_write_mask(struct pdbg_target *target, uint64_t addr, uint64_t val, uint64_t mask);
int pib_wait(struct pdbg_target *pib_dt, uint64_t addr, uint64_t mask, uint64_t data);

struct thread_regs {
	uint64_t nia;
	uint64_t msr;
	uint64_t cfar;
	uint64_t lr;
	uint64_t ctr;
	uint64_t tar;
	uint32_t cr;
	uint64_t xer;
	uint64_t gprs[32];

	uint64_t lpcr;
	uint64_t ptcr;
	uint64_t lpidr;
	uint64_t pidr;
	uint64_t hfscr;
	uint32_t hdsisr;
	uint64_t hdar;
	uint64_t hsrr0;
	uint64_t hsrr1;
	uint64_t hdec;
	uint32_t heir;
	uint64_t hid;
	uint64_t hsprg0;
	uint64_t hsprg1;
	uint64_t fscr;
	uint32_t dsisr;
	uint64_t dar;
	uint64_t srr0;
	uint64_t srr1;
	uint64_t dec;
	uint64_t tb;
	uint64_t sprg0;
	uint64_t sprg1;
	uint64_t sprg2;
	uint64_t sprg3;
	uint64_t ppr;
};

int thread_putmsr(struct pdbg_target *target, uint64_t val);
int thread_getmem(struct pdbg_target *thread, uint64_t addr, uint64_t *value);
int thread_putnia(struct pdbg_target *target, uint64_t val);
int thread_putspr(struct pdbg_target *target, int spr, uint64_t val);
int thread_putgpr(struct pdbg_target *target, int spr, uint64_t val);
int thread_getmsr(struct pdbg_target *target, uint64_t *val);
int thread_getcr(struct pdbg_target *thread,  uint32_t *value);
int thread_putcr(struct pdbg_target *thread,  uint32_t value);
int thread_getnia(struct pdbg_target *target, uint64_t *val);
int thread_getspr(struct pdbg_target *target, int spr, uint64_t *val);
int thread_getgpr(struct pdbg_target *target, int gpr, uint64_t *val);
int thread_getxer(struct pdbg_target *thread, uint64_t *value);
int thread_putxer(struct pdbg_target *thread, uint64_t value);
int thread_getregs(struct pdbg_target *target, struct thread_regs *regs);

enum pdbg_sleep_state {PDBG_THREAD_STATE_RUN, PDBG_THREAD_STATE_DOZE,
		       PDBG_THREAD_STATE_NAP, PDBG_THREAD_STATE_SLEEP,
		       PDBG_THREAD_STATE_STOP};

enum pdbg_smt_state {PDBG_SMT_UNKNOWN, PDBG_SMT_1, PDBG_SMT_2, PDBG_SMT_4, PDBG_SMT_8};

struct thread_state {
	bool active;
	bool quiesced;
	enum pdbg_sleep_state sleep_state;
	enum pdbg_smt_state smt_state;
};

int thread_start(struct pdbg_target *target);
int thread_step(struct pdbg_target *target, int steps);
int thread_stop(struct pdbg_target *target);
int thread_sreset(struct pdbg_target *target);
int thread_start_all(void);
int thread_step_all(void);
int thread_stop_all(void);
int thread_sreset_all(void);
struct thread_state thread_status(struct pdbg_target *target);

int getring(struct pdbg_target *chiplet_target, uint64_t ring_addr, uint64_t ring_len, uint32_t result[]);

int htm_start(struct pdbg_target *target);
int htm_stop(struct pdbg_target *target);
int htm_status(struct pdbg_target *target);
int htm_dump(struct pdbg_target *target, char *filename);
int htm_record(struct pdbg_target *target, char *filename);

int adu_getmem(struct pdbg_target *target, uint64_t addr,
	       uint8_t *ouput, uint64_t size);
int adu_putmem(struct pdbg_target *target, uint64_t addr,
	       uint8_t *input, uint64_t size);
int adu_getmem_ci(struct pdbg_target *target, uint64_t addr,
		  uint8_t *ouput, uint64_t size);
int adu_putmem_ci(struct pdbg_target *target, uint64_t addr,
		  uint8_t *input, uint64_t size);
int adu_getmem_io(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *output, uint64_t size, uint8_t blocksize);
int adu_putmem_io(struct pdbg_target *adu_target, uint64_t start_addr,
		  uint8_t *input, uint64_t size, uint8_t block_size);
int __adu_getmem(struct pdbg_target *target, uint64_t addr, uint8_t *ouput,
		 uint64_t size, bool ci);
int __adu_putmem(struct pdbg_target *target, uint64_t addr, uint8_t *input,
		 uint64_t size, bool ci);

int mem_read(struct pdbg_target *target, uint64_t addr, uint8_t *output, uint64_t size, uint8_t block_size, bool ci);
int mem_write(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size, uint8_t block_size, bool ci);

int opb_read(struct pdbg_target *target, uint32_t addr, uint32_t *data);
int opb_write(struct pdbg_target *target, uint32_t addr, uint32_t data);

int sbe_istep(struct pdbg_target *target, uint32_t major, uint32_t minor);
uint32_t sbe_ffdc_get(struct pdbg_target *target, const uint8_t **ffdc, uint32_t *ffdc_len);

typedef void (*pdbg_progress_tick_t)(uint64_t cur, uint64_t end);

void pdbg_set_progress_tick(pdbg_progress_tick_t fn);
void pdbg_progress_tick(uint64_t cur, uint64_t end);

#define PDBG_ERROR	0
#define PDBG_WARNING	1
#define PDBG_NOTICE	2
#define PDBG_INFO	3
#define PDBG_DEBUG	4

typedef void (*pdbg_log_func_t)(int loglevel, const char *fmt, va_list ap);

void pdbg_set_logfunc(pdbg_log_func_t fn);
void pdbg_set_loglevel(int loglevel);
void pdbg_log(int loglevel, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
