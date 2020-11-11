#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <ccan/list/list.h>
#include <libfdt/libfdt.h>

#include "bitutils.h"
#include "hwunit.h"
#include "operations.h"
#include "debug.h"

struct list_head empty_list = LIST_HEAD_INIT(empty_list);
struct list_head target_classes = LIST_HEAD_INIT(target_classes);

/* Work out the address to access based on the current target and
 * final class name */
static struct pdbg_target *get_class_target_addr(struct pdbg_target *target, const char *name, uint64_t *addr)
{
	uint64_t old_addr = *addr;

	/* Check class */
	while (strcmp(target->class, name)) {
		if (target->translate) {
			*addr = target->translate(target, *addr);
			target = target_parent(name, target, false);
			assert(target);
			break;
		} else {
			*addr += pdbg_target_address(target, NULL);
		}

		/* Keep walking the tree translating addresses */
		target = get_parent(target, false);

		/* The root node doesn't have an address space so it's
		 * an error in the device tree if we hit this. */
		assert(target != pdbg_target_root());
	}

	pdbg_log(PDBG_DEBUG, "Translating target addr 0x%" PRIx64 " -> 0x%" PRIx64 " on %s\n",
		 old_addr, *addr, pdbg_target_path(target));
	return target;
}

struct pdbg_target *pdbg_address_absolute(struct pdbg_target *target, uint64_t *addr)
{
	return get_class_target_addr(target, "pib", addr);
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

	if (pdbg_target_status(pib_dt) != PDBG_TARGET_ENABLED)
		return -1;

	pib = target_to_pib(pib_dt);

	if (!pib->read) {
		PR_ERROR("read() not implemented for the target\n");
		return -1;
	}

	if (target_addr & PPC_BIT(0))
		rc = pib_indirect_read(pib, target_addr, data);
	else
		rc = pib->read(pib, target_addr, data);

	PR_DEBUG("rc = %d, addr = 0x%016" PRIx64 ", data = 0x%016" PRIx64 ", target = %s\n",
		 rc, target_addr, *data, pdbg_target_path(&pib->target));

	return rc;
}

int pib_write(struct pdbg_target *pib_dt, uint64_t addr, uint64_t data)
{
	struct pib *pib;
	uint64_t target_addr = addr;
	int rc;

	pib_dt = get_class_target_addr(pib_dt, "pib", &target_addr);

	if (pdbg_target_status(pib_dt) != PDBG_TARGET_ENABLED)
		return -1;

	pib = target_to_pib(pib_dt);

	if (!pib->write) {
		PR_ERROR("write() not implemented for the target\n");
		return -1;
	}

	PR_DEBUG("addr:0x%08" PRIx64 " data:0x%016" PRIx64 "\n",
		 target_addr, data);
	if (target_addr & PPC_BIT(0))
		rc = pib_indirect_write(pib, target_addr, data);
	else
		rc = pib->write(pib, target_addr, data);

	PR_DEBUG("rc = %d, addr = 0x%016" PRIx64 ", data = 0x%016" PRIx64 ", target = %s\n",
		 rc, target_addr, data, pdbg_target_path(&pib->target));

	return rc;
}

int pib_write_mask(struct pdbg_target *pib_dt, uint64_t addr, uint64_t data, uint64_t mask)
{
	uint64_t value;
	int rc;

	rc = pib_read(pib_dt, addr, &value);
	if (rc)
		return rc;

	value = (value & ~mask) | (data & mask);

	return pib_write(pib_dt, addr, value);
}

/* Wait for a SCOM register addr to match value & mask == data */
int pib_wait(struct pdbg_target *pib_dt, uint64_t addr, uint64_t mask, uint64_t data)
{
	struct pib *pib;
	uint64_t tmp;
	int rc;

	pib_dt = get_class_target_addr(pib_dt, "pib", &addr);

	if (pdbg_target_status(pib_dt) != PDBG_TARGET_ENABLED)
		return -1;

	pib = target_to_pib(pib_dt);

	if (!pib->read) {
		PR_ERROR("read() not implemented for the target\n");
		return -1;
	}

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

	if (pdbg_target_status(opb_dt) != PDBG_TARGET_ENABLED)
		return -1;

	opb = target_to_opb(opb_dt);

	if (!opb->read) {
		PR_ERROR("read() not implemented for the target\n");
		return -1;
	}

	return opb->read(opb, addr64, data);
}

int opb_write(struct pdbg_target *opb_dt, uint32_t addr, uint32_t data)
{
	struct opb *opb;
	uint64_t addr64 = addr;

	opb_dt = get_class_target_addr(opb_dt, "opb", &addr64);

	if (pdbg_target_status(opb_dt) != PDBG_TARGET_ENABLED)
		return -1;

	opb = target_to_opb(opb_dt);

	if (!opb->write) {
		PR_ERROR("write() not implemented for the target\n");
		return -1;
	}
	return opb->write(opb, addr64, data);
}

int fsi_read(struct pdbg_target *fsi_dt, uint32_t addr, uint32_t *data)
{
	struct fsi *fsi;
	int rc;
	uint64_t addr64 = addr;

	fsi_dt = get_class_target_addr(fsi_dt, "fsi", &addr64);
	fsi = target_to_fsi(fsi_dt);

	if (!fsi->read) {
		PR_ERROR("read() not implemented for the target\n");
		return -1;
	}

	rc = fsi->read(fsi, addr64, data);
	PR_DEBUG("rc = %d, addr = 0x%05" PRIx64 ", data = 0x%08" PRIx32 ", target = %s\n",
		 rc, addr64, *data, pdbg_target_path(&fsi->target));
	return rc;
}

int fsi_write(struct pdbg_target *fsi_dt, uint32_t addr, uint32_t data)
{
	struct fsi *fsi;
	int rc;
	uint64_t addr64 = addr;

	fsi_dt = get_class_target_addr(fsi_dt, "fsi", &addr64);
	fsi = target_to_fsi(fsi_dt);

	if (!fsi->write) {
		PR_ERROR("write() not implemented for the target\n");
		return -1;
	}

	rc = fsi->write(fsi, addr64, data);
	PR_DEBUG("rc = %d, addr = 0x%05" PRIx64 ", data = 0x%08" PRIx32 ", target = %s\n",
		 rc, addr64, data, pdbg_target_path(&fsi->target));
	return rc;
}

int fsi_write_mask(struct pdbg_target *fsi_dt, uint32_t addr, uint32_t data, uint32_t mask)
{
	uint32_t value;
	int rc;

	rc = fsi_read(fsi_dt, addr, &value);
	if (rc)
		return rc;

	value = (value & ~mask) | (data & mask);

	return fsi_write(fsi_dt, addr, value);
}

int mem_read(struct pdbg_target *target, uint64_t addr, uint8_t *output, uint64_t size, uint8_t block_size, bool ci)
{
	struct mem *mem;
	int rc = -1;

	assert(pdbg_target_is_class(target, "mem"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	mem = target_to_mem(target);

	if (!mem->read) {
		PR_ERROR("read() not implemented for the target\n");
		return -1;
	}

	rc = mem->read(mem, addr, output, size, block_size, ci);

	return rc;
}

int mem_write(struct pdbg_target *target, uint64_t addr, uint8_t *input, uint64_t size, uint8_t block_size, bool ci)
{
	struct mem *mem;
	int rc = -1;

	assert(pdbg_target_is_class(target, "mem"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	mem = target_to_mem(target);

	if (!mem->write) {
		PR_ERROR("write() not implemented for the target\n");
		return -1;
	}

	rc = mem->write(mem, addr, input, size, block_size, ci);

	return rc;
}

struct chipop *pib_to_chipop(struct pdbg_target *pib)
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

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->istep) {
		PR_ERROR("istep() not implemented for the target\n");
		return -1;
	}

	return chipop->istep(chipop, major, minor);
}

int sbe_mpipl_enter(struct pdbg_target *target)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_enter) {
		PR_ERROR("mpipl_enter() not implemented for the target\n");
		return -1;
	}

	return chipop->mpipl_enter(chipop);
}

int sbe_mpipl_continue(struct pdbg_target *target)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_continue) {
		PR_ERROR("mpipl_continue() not implemented for the target\n");
		return -1;
	}

	return chipop->mpipl_continue(chipop);
}

int sbe_mpipl_get_ti_info(struct pdbg_target *target, uint8_t **data, uint32_t *data_len)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->mpipl_get_ti_info) {
		PR_ERROR("mpipl_get_ti_info() not implemented for the target\n");
		return -1;
	}

	return chipop->mpipl_get_ti_info(chipop, data, data_len);
}

int sbe_dump(struct pdbg_target *target, uint8_t type, uint8_t clock, uint8_t **data, uint32_t *data_len)
{
	struct chipop *chipop;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->dump) {
		PR_ERROR("dump() not implemented for the target\n");
		return -1;
	}

	return chipop->dump(chipop, type, clock, data, data_len);
}

int ocmb_getscom(struct pdbg_target *target, uint64_t addr, uint64_t *val)
{
	struct ocmb *ocmb;

	assert(pdbg_target_is_class(target, "ocmb"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	ocmb = target_to_ocmb(target);

	if (!ocmb->getscom) {
		PR_ERROR("getscom() not implemented for the target\n");
		return -1;
	}

	return ocmb->getscom(ocmb, addr, val);
}

int ocmb_putscom(struct pdbg_target *target, uint64_t addr, uint64_t val)
{
	struct ocmb *ocmb;

	assert(pdbg_target_is_class(target, "ocmb"));

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return -1;

	ocmb = target_to_ocmb(target);

	if (!ocmb->putscom) {
		PR_ERROR("putscom() not implemented for the target\n");
		return -1;
	}

	return ocmb->putscom(ocmb, addr, val);
}

uint32_t sbe_ffdc_get(struct pdbg_target *target, uint8_t **ffdc, uint32_t *ffdc_len)
{
	struct chipop *chipop;
	const uint8_t *data = NULL;
	uint32_t status, len = 0;

	chipop = pib_to_chipop(target);
	if (!chipop)
		return -1;

	if (!chipop->ffdc_get) {
		PR_ERROR("ffdc_get() not implemented for the target\n");
		return -1;
	}

	status = chipop->ffdc_get(chipop, &data, &len);
	if (data && len > 0) {
		*ffdc = malloc(len);
		assert(*ffdc);
		memcpy(*ffdc, data, len);
		*ffdc_len = len;
	} else {
		*ffdc = NULL;
		*ffdc_len = 0;
	}

	return status;
}

/* Finds the given class. Returns NULL if not found. */
struct pdbg_target_class *find_target_class(const char *name)
{
	struct pdbg_target_class *target_class = NULL;

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
struct pdbg_target_class *get_target_class(struct pdbg_target *target)
{
	struct pdbg_target_class *target_class;

	if ((target_class = find_target_class(target->class)))
		return target_class;

	/* Need to allocate a new class */
	PR_DEBUG("Allocating %s target class\n", target->class);
	target_class = calloc(1, sizeof(*target_class));
	assert(target_class);
	target_class->name = strdup(target->class);
	list_head_init(&target_class->targets);
	list_add_tail(&target_classes, &target_class->class_head_link);
	return target_class;
}

/* We walk the tree root down disabling targets which might/should
 * exist but don't */
enum pdbg_target_status pdbg_target_probe(struct pdbg_target *target)
{
	struct pdbg_target *parent, *vnode;
	enum pdbg_target_status status;

	assert(target);

	status = pdbg_target_status(target);
	assert(status != PDBG_TARGET_RELEASED);

	if (status == PDBG_TARGET_DISABLED || status == PDBG_TARGET_NONEXISTENT
	    || status == PDBG_TARGET_ENABLED)
		/* We've already tried probing this target and by assumption
		 * it's status won't have changed */
		   return status;

	parent = get_parent(target, false);
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

	/* Make sure any virtual nodes are also probed */
	vnode = target_to_virtual(target, true);
	if (vnode)
		pdbg_target_probe(vnode);

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

/* Releases a target by first recursively releasing all its children */
void pdbg_target_release(struct pdbg_target *target)
{
	struct pdbg_target *child;

	if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
		return;

	pdbg_for_each_child_target(target, child)
		pdbg_target_release(child);

	/* Release the target */
	if (target->release)
		target->release(target);
	target->status = PDBG_TARGET_RELEASED;
}

/*
 * Probe all targets in the device tree.
 */
void pdbg_target_probe_all(struct pdbg_target *parent)
{
	struct pdbg_target *child;

	if (!parent)
		parent = pdbg_target_root();

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

/* For virtual nodes, compatible property is not set */
bool target_is_virtual(struct pdbg_target *target)
{
	return (!target->compatible);
}

/* Map virtual target to real target */
struct pdbg_target *target_to_real(struct pdbg_target *target, bool strict)
{
	if (!target->compatible && target->vnode)
		return target->vnode;

	if (strict)
		return NULL;

	return target;
}

/* Map real target to virtual target */
struct pdbg_target *target_to_virtual(struct pdbg_target *target, bool strict)
{
	if (target->compatible && target->vnode)
		return target->vnode;

	if (strict)
		return NULL;

	return target;
}
