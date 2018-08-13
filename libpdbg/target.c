#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <ccan/list/list.h>
#include <libfdt/libfdt.h>

#include "bitutils.h"
#include "target.h"
#include "device.h"
#include "operations.h"
#include "debug.h"

struct list_head empty_list = LIST_HEAD_INIT(empty_list);
struct list_head target_classes = LIST_HEAD_INIT(target_classes);

/* Work out the address to access based on the current target and
 * final class name */
static struct pdbg_target *get_class_target_addr(struct pdbg_target *target, const char *name, uint64_t *addr)
{
	/* Check class */
	while (strcmp(target->class, name)) {
		/* Keep walking the tree translating addresses */
		*addr += dt_get_address(target, 0, NULL);
		target = target->parent;

		/* The should always be a parent. If there isn't it
		 * means we traversed up the whole device tree and
		 * didn't find a parent matching the given class. */
		assert(target);
	}

	return target;
}

/* The indirect access code was largely stolen from hw/xscom.c in skiboot */
#define PIB_IND_MAX_RETRIES 10
#define PIB_IND_READ PPC_BIT(0)
#define PIB_IND_ADDR PPC_BITMASK(12, 31)
#define PIB_IND_DATA PPC_BITMASK(48, 63)

#define PIB_DATA_IND_COMPLETE PPC_BIT(32)
#define PIB_DATA_IND_ERR PPC_BITMASK(33, 35)
#define PIB_DATA_IND_DATA PPC_BITMASK(48, 63)

static int pib_indirect_read(struct pib *pib, uint64_t addr, uint64_t *data)
{
	uint64_t indirect_addr;
	int retries;

	if ((addr >> 60) & 1) {
		PR_ERROR("Indirect form 1 not supported\n");
		return -1;
	}

	indirect_addr = addr & 0x7fffffff;
	*data = PIB_IND_READ | (addr & PIB_IND_ADDR);
	CHECK_ERR(pib->write(pib, indirect_addr, *data));

	/* Wait for completion */
	for (retries = 0; retries < PIB_IND_MAX_RETRIES; retries++) {
		CHECK_ERR(pib->read(pib, indirect_addr, data));

		if ((*data & PIB_DATA_IND_COMPLETE) &&
		    ((*data & PIB_DATA_IND_ERR) == 0)) {
			*data = *data & PIB_DATA_IND_DATA;
			break;
		}

		if ((*data & PIB_DATA_IND_COMPLETE) ||
		    (retries >= PIB_IND_MAX_RETRIES)) {
			PR_ERROR("Error reading indirect register");
			return -1;
		}
	}

	return 0;
}

static int pib_indirect_write(struct pib *pib, uint64_t addr, uint64_t data)
{
	uint64_t indirect_addr;
	int retries;

	if ((addr >> 60) & 1) {
		PR_ERROR("Indirect form 1 not supported\n");
		return -1;
	}

	indirect_addr = addr & 0x7fffffff;
	data &= PIB_IND_DATA;
	data |= addr & PIB_IND_ADDR;
	CHECK_ERR(pib->write(pib, indirect_addr, data));

	/* Wait for completion */
	for (retries = 0; retries < PIB_IND_MAX_RETRIES; retries++) {
		CHECK_ERR(pib->read(pib, indirect_addr, &data));

		if ((data & PIB_DATA_IND_COMPLETE) &&
		    ((data & PIB_DATA_IND_ERR) == 0))
			break;

		if ((data & PIB_DATA_IND_COMPLETE) ||
		    (retries >= PIB_IND_MAX_RETRIES)) {
			PR_ERROR("Error writing indirect register");
			return -1;
		}
	}

	return 0;
}

int pib_read(struct pdbg_target *pib_dt, uint64_t addr, uint64_t *data)
{
	struct pib *pib;
	uint64_t target_addr = addr;
	int rc;

	pib_dt = get_class_target_addr(pib_dt, "pib", &target_addr);
	pib = target_to_pib(pib_dt);
	if (target_addr & PPC_BIT(0))
		rc = pib_indirect_read(pib, target_addr, data);
	else
		rc = pib->read(pib, target_addr, data);
	PR_DEBUG("addr:0x%08" PRIx64 " data:0x%016" PRIx64 "\n",
		 target_addr, *data);
	return rc;
}

int pib_write(struct pdbg_target *pib_dt, uint64_t addr, uint64_t data)
{
	struct pib *pib;
	uint64_t target_addr = addr;
	int rc;

	pib_dt = get_class_target_addr(pib_dt, "pib", &target_addr);
	pib = target_to_pib(pib_dt);
	PR_DEBUG("addr:0x%08" PRIx64 " data:0x%016" PRIx64 "\n",
		 target_addr, data);
	if (target_addr & PPC_BIT(0))
		rc = pib_indirect_write(pib, target_addr, data);
	else
		rc = pib->write(pib, target_addr, data);
	return rc;
}

/* Wait for a SCOM register addr to match value & mask == data */
int pib_wait(struct pdbg_target *pib_dt, uint64_t addr, uint64_t mask, uint64_t data)
{
	struct pib *pib;
	uint64_t tmp;
	int rc;

	pib_dt = get_class_target_addr(pib_dt, "pib", &addr);
	pib = target_to_pib(pib_dt);

	do {
		if (addr & PPC_BIT(0))
			rc = pib_indirect_read(pib, addr, &tmp);
		else
			rc = pib->read(pib, addr, &tmp);
		if (rc)
			return rc;
	} while ((tmp & mask) != data);

	return 0;
}

int opb_read(struct pdbg_target *opb_dt, uint32_t addr, uint32_t *data)
{
	struct opb *opb;
	uint64_t addr64 = addr;

	opb_dt = get_class_target_addr(opb_dt, "opb", &addr64);
	opb = target_to_opb(opb_dt);
	return opb->read(opb, addr64, data);
}

int opb_write(struct pdbg_target *opb_dt, uint32_t addr, uint32_t data)
{
	struct opb *opb;
	uint64_t addr64 = addr;

	opb_dt = get_class_target_addr(opb_dt, "opb", &addr64);
	opb = target_to_opb(opb_dt);

	return opb->write(opb, addr64, data);
}

int fsi_read(struct pdbg_target *fsi_dt, uint32_t addr, uint32_t *data)
{
	struct fsi *fsi;
	uint64_t addr64 = addr;

	fsi_dt = get_class_target_addr(fsi_dt, "fsi", &addr64);
	fsi = target_to_fsi(fsi_dt);
	return fsi->read(fsi, addr64, data);
}

int fsi_write(struct pdbg_target *fsi_dt, uint32_t addr, uint32_t data)
{
	struct fsi *fsi;
	uint64_t addr64 = addr;

	fsi_dt = get_class_target_addr(fsi_dt, "fsi", &addr64);
	fsi = target_to_fsi(fsi_dt);

	return fsi->write(fsi, addr64, data);
}

struct pdbg_target *require_target_parent(struct pdbg_target *target)
{
	assert(target->parent);
	return target->parent;
}

/* Finds the given class. Returns NULL if not found. */
struct pdbg_target_class *find_target_class(const char *name)
{
	struct pdbg_target_class *target_class;

	list_for_each(&target_classes, target_class, class_head_link)
		if (!strcmp(target_class->name, name))
			return target_class;

	return NULL;
}

/* Same as above but dies with an assert if the target class doesn't
 * exist */
struct pdbg_target_class *require_target_class(const char *name)
{
	struct pdbg_target_class *target_class;

	target_class = find_target_class(name);
	if (!target_class) {
		PR_ERROR("Couldn't find class %s\n", name);
		assert(0);
	}
	return target_class;
}

/* Returns the existing class or allocates space for a new one */
struct pdbg_target_class *get_target_class(const char *name)
{
	struct pdbg_target_class *target_class;

	if ((target_class = find_target_class(name)))
		return target_class;

	/* Need to allocate a new class */
	PR_DEBUG("Allocating %s target class\n", name);
	target_class = calloc(1, sizeof(*target_class));
	assert(target_class);
	target_class->name = strdup(name);
	list_head_init(&target_class->targets);
	list_add_tail(&target_classes, &target_class->class_head_link);
	return target_class;
}

extern struct hw_unit_info *__start_hw_units;
extern struct hw_init_info *__stop_hw_units;
struct hw_unit_info *find_compatible_target(const char *compat)
{
	struct hw_unit_info **p;
	struct pdbg_target *target;

	for (p = &__start_hw_units; p < (struct hw_unit_info **) &__stop_hw_units; p++) {
		target = (*p)->hw_unit;
		if (!strcmp(target->compatible, compat))
			return *p;
	}

	return NULL;
}

void pdbg_targets_init(void *fdt)
{
	dt_root = dt_new_root("", NULL, 0);
	dt_expand(fdt);
}

/* We walk the tree root down disabling targets which might/should
 * exist but don't */
enum pdbg_target_status pdbg_target_probe(struct pdbg_target *target)
{
	struct pdbg_target *parent;
	enum pdbg_target_status status;

	assert(target);

	status = pdbg_target_status(target);
	assert(status != PDBG_TARGET_RELEASED);

	if (status == PDBG_TARGET_DISABLED || status == PDBG_TARGET_NONEXISTENT
	    || status == PDBG_TARGET_ENABLED)
		/* We've already tried probing this target and by assumption
		 * it's status won't have changed */
		   return status;

	parent = target->parent;
	if (parent) {
		/* Recurse up the tree to probe and set parent target status */
		pdbg_target_probe(parent);
		status = pdbg_target_status(parent);
		switch(status) {
		case PDBG_TARGET_NONEXISTENT:
			/* The parent doesn't exist neither does it's
			 * children */
			target->status = PDBG_TARGET_NONEXISTENT;
			return PDBG_TARGET_NONEXISTENT;

		case PDBG_TARGET_DISABLED:
			/* The parent is disabled, we know nothing of the child
			 * so leave it in it's current state unless it must
			 * exist.
			 * TODO: Must exist error reporting */
			assert(pdbg_target_status(target) != PDBG_TARGET_MUSTEXIST);
			return pdbg_target_status(target);

		case PDBG_TARGET_RELEASED:
		case PDBG_TARGET_MUSTEXIST:
		case PDBG_TARGET_UNKNOWN:
			/* We must know by now if the parent exists or not */
			assert(0);
			break;

		case PDBG_TARGET_ENABLED:
			break;
		}
	}

	/* At this point any parents must exist and have already been probed */
	if (target->probe && target->probe(target)) {
		/* Could not find the target */
		assert(pdbg_target_status(target) != PDBG_TARGET_MUSTEXIST);
		target->status = PDBG_TARGET_NONEXISTENT;
		return PDBG_TARGET_NONEXISTENT;
	}

	target->status = PDBG_TARGET_ENABLED;
	return PDBG_TARGET_ENABLED;
}

/*
 * Walk back up, releasing.
 */
void pdbg_target_release(struct pdbg_target *target)
{
	struct pdbg_target *parent;
	struct pdbg_target *child;

	assert(target);

	/* If it's not enabled, the parent wasn't enabled. */
	if ((pdbg_target_status(target) != PDBG_TARGET_ENABLED))
		return;

	pdbg_for_each_child_target(target, child) {
		/* Not all children released yet, stop here. */
		if (pdbg_target_status(child) == PDBG_TARGET_ENABLED)
			return;
	}

	/* At this point any parents must exist and have already been probed */
	if (target->release)
		target->release(target);
	target->status = PDBG_TARGET_RELEASED;

	parent = target->parent;
	if (parent) {
		/* Recurse up the tree to release parents */
		pdbg_target_release(parent);
	}
}

/*
 * Probe all targets in the device tree.
 */
void pdbg_target_probe_all(struct pdbg_target *parent)
{
	struct pdbg_target *child;

	if (!parent)
		parent = dt_root;

	pdbg_for_each_child_target(parent, child) {
		pdbg_target_probe_all(child);
		pdbg_target_probe(child);
	}
}

bool pdbg_target_is_class(struct pdbg_target *target, const char *class)
{
	if (!target || !target->class || !class)
		return false;
	return strcmp(target->class, class) == 0;
}

void *pdbg_target_priv(struct pdbg_target *target)
{
	return target->priv;
}

void pdbg_target_priv_set(struct pdbg_target *target, void *priv)
{
	target->priv = priv;
}
