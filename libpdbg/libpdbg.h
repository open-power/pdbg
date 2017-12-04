#ifndef __LIBPDBG_H
#define __LIBPDBG_H

struct pdbg_target;
struct pdbg_target_class;

struct pdbg_taget *pdbg_root_target;

/* loops/iterators */
struct pdbg_target *__pdbg_next_target(const char *klass, struct pdbg_target *parent, struct pdbg_target *last);
struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last);
enum pdbg_target_status {PDBG_TARGET_ENABLED, PDBG_TARGET_DISABLED, PDBG_TARGET_HIDDEN};

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

/* Set the given property. Will automatically add one if one doesn't exist */
void pdbg_set_target_property(struct pdbg_target *target, const char *name, const void *val, size_t size);

/* Get the given property and return the size */
void *pdbg_get_target_property(struct pdbg_target *target, const char *name, size_t *size);
int pdbg_get_u64_property(struct pdbg_target *target, const char *name, uint64_t *val);
uint64_t pdbg_get_address(struct pdbg_target *target, uint64_t *size);

/* Misc. */
void pdbg_targets_init(void *fdt);
void pdbg_target_probe(void);
void pdbg_enable_target(struct pdbg_target *target);
void pdbg_disable_target(struct pdbg_target *target);
enum pdbg_target_status pdbg_target_status(struct pdbg_target *target);
uint32_t pdbg_target_index(struct pdbg_target *target);
uint32_t pdbg_parent_index(struct pdbg_target *target, char *klass);
char *pdbg_target_class_name(struct pdbg_target *target);
char *pdbg_target_name(struct pdbg_target *target);

/* Procedures */
int fsi_read(struct pdbg_target *target, uint32_t addr, uint32_t *val);
int fsi_write(struct pdbg_target *target, uint32_t addr, uint32_t val);

int pib_read(struct pdbg_target *target, uint64_t addr, uint64_t *val);
int pib_write(struct pdbg_target *target, uint64_t addr, uint64_t val);

int ram_putmsr(struct pdbg_target *target, uint64_t val);
int ram_putnia(struct pdbg_target *target, uint64_t val);
int ram_putspr(struct pdbg_target *target, int spr, uint64_t val);
int ram_putgpr(struct pdbg_target *target, int spr, uint64_t val);
int ram_getmsr(struct pdbg_target *target, uint64_t *val);
int ram_getnia(struct pdbg_target *target, uint64_t *val);
int ram_getspr(struct pdbg_target *target, int spr, uint64_t *val);
int ram_getgpr(struct pdbg_target *target, int gpr, uint64_t *val);
int ram_start_thread(struct pdbg_target *target);
int ram_step_thread(struct pdbg_target *target, int steps);
int ram_stop_thread(struct pdbg_target *target);
int ram_sreset_thread(struct pdbg_target *target);
uint64_t thread_status(struct pdbg_target *target);

#define THREAD_STATUS_DISABLED  PPC_BIT(0)
#define THREAD_STATUS_ACTIVE       PPC_BIT(63)
#define THREAD_STATUS_STATE        PPC_BITMASK(61, 62)
#define THREAD_STATUS_DOZE PPC_BIT(62)
#define THREAD_STATUS_NAP  PPC_BIT(61)
#define THREAD_STATUS_SLEEP        PPC_BITMASK(61, 62)
#define THREAD_STATUS_QUIESCE      PPC_BIT(60)

int htm_start(struct pdbg_target *target);
int htm_stop(struct pdbg_target *target);
int htm_status(struct pdbg_target *target);
int htm_reset(struct pdbg_target *target, uint64_t *base, uint64_t *size);
int htm_dump(struct pdbg_target *target, uint64_t size, const char *filename);

int adu_getmem(struct pdbg_target *target, uint64_t addr, uint8_t *ouput, uint64_t size);
int adu_putmem(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size);

int opb_read(struct pdbg_target *target, uint32_t addr, uint32_t *data);
int opb_write(struct pdbg_target *target, uint32_t addr, uint32_t data);

#endif
