#ifndef __LIBPDBG_H
#define __LIBPDBG_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <stdbool.h>

struct pdbg_target;
struct pdbg_target_class;

/* loops/iterators */
struct pdbg_target *__pdbg_next_target(const char *klass, struct pdbg_target *parent, struct pdbg_target *last);
struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last);

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

#define pdbg_for_each_target(class, parent, target)			\
	for (target = __pdbg_next_target(class, parent, NULL);		\
	     target;							\
	     target = __pdbg_next_target(class, parent, target))

#define pdbg_for_each_class_target(class, target)		\
	for (target = __pdbg_next_target(class, NULL, NULL);	\
	     target;						\
	     target = __pdbg_next_target(class, NULL, target))

#define pdbg_for_each_child_target(parent, target)	      \
	for (target = __pdbg_next_child_target(parent, NULL); \
	     target;					      \
	     target = __pdbg_next_child_target(parent, target))

/* Return the first parent target of the given class, or NULL if the given
 * target does not have a parent of the given class. */
struct pdbg_target *pdbg_target_parent(const char *klass, struct pdbg_target *target);

/* Same as above but instead of returning NULL causes an assert failure. */
struct pdbg_target *pdbg_target_require_parent(const char *klass, struct pdbg_target *target);

/* Set the given property. Will automatically add one if one doesn't exist */
void pdbg_set_target_property(struct pdbg_target *target, const char *name, const void *val, size_t size);

/* Get the given property and return the size */
void *pdbg_get_target_property(struct pdbg_target *target, const char *name, size_t *size);
int pdbg_get_u64_property(struct pdbg_target *target, const char *name, uint64_t *val);
uint64_t pdbg_get_address(struct pdbg_target *target, uint64_t *size);

/* Misc. */
void pdbg_targets_init(void *fdt);
void pdbg_target_probe_all(struct pdbg_target *parent);
enum pdbg_target_status pdbg_target_probe(struct pdbg_target *target);
void pdbg_target_release(struct pdbg_target *target);
enum pdbg_target_status pdbg_target_status(struct pdbg_target *target);
void pdbg_target_status_set(struct pdbg_target *target, enum pdbg_target_status status);
uint32_t pdbg_target_index(struct pdbg_target *target);
uint32_t pdbg_parent_index(struct pdbg_target *target, char *klass);
char *pdbg_target_class_name(struct pdbg_target *target);
char *pdbg_target_name(struct pdbg_target *target);
const char *pdbg_target_dn_name(struct pdbg_target *target);
void *pdbg_target_priv(struct pdbg_target *target);
void pdbg_target_priv_set(struct pdbg_target *target, void *priv);
struct pdbg_target *pdbg_target_root(void);

/* Procedures */
int fsi_read(struct pdbg_target *target, uint32_t addr, uint32_t *val);
int fsi_write(struct pdbg_target *target, uint32_t addr, uint32_t val);

int pib_read(struct pdbg_target *target, uint64_t addr, uint64_t *val);
int pib_write(struct pdbg_target *target, uint64_t addr, uint64_t val);
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

int ram_putmsr(struct pdbg_target *target, uint64_t val);
int ram_putnia(struct pdbg_target *target, uint64_t val);
int ram_putspr(struct pdbg_target *target, int spr, uint64_t val);
int ram_putgpr(struct pdbg_target *target, int spr, uint64_t val);
int ram_getmsr(struct pdbg_target *target, uint64_t *val);
int ram_getcr(struct pdbg_target *thread,  uint32_t *value);
int ram_getnia(struct pdbg_target *target, uint64_t *val);
int ram_getspr(struct pdbg_target *target, int spr, uint64_t *val);
int ram_getgpr(struct pdbg_target *target, int gpr, uint64_t *val);
int ram_start_thread(struct pdbg_target *target);
int ram_step_thread(struct pdbg_target *target, int steps);
int ram_stop_thread(struct pdbg_target *target);
int ram_sreset_thread(struct pdbg_target *target);
int ram_state_thread(struct pdbg_target *target, struct thread_regs *regs);
struct thread_state thread_status(struct pdbg_target *target);
int ram_getxer(struct pdbg_target *thread, uint64_t *value);
int ram_putxer(struct pdbg_target *thread, uint64_t value);
int getring(struct pdbg_target *chiplet_target, uint64_t ring_addr, uint64_t ring_len, uint32_t result[]);

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

int htm_start(struct pdbg_target *target);
int htm_stop(struct pdbg_target *target);
int htm_status(struct pdbg_target *target);
int htm_dump(struct pdbg_target *target, char *filename);
int htm_record(struct pdbg_target *target, char *filename);

int adu_getmem(struct pdbg_target *target, uint64_t addr, uint8_t *ouput, uint64_t size);
int adu_putmem(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size);
int adu_getmem_ci(struct pdbg_target *target, uint64_t addr, uint8_t *ouput, uint64_t size);
int adu_putmem_ci(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size);
int __adu_getmem(struct pdbg_target *target, uint64_t addr, uint8_t *ouput, uint64_t size, bool ci);
int __adu_putmem(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size, bool ci);

int opb_read(struct pdbg_target *target, uint32_t addr, uint32_t *data);
int opb_write(struct pdbg_target *target, uint32_t addr, uint32_t data);

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

#endif
