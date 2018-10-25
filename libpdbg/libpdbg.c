#include <string.h>

#include "target.h"
#include "device.h"
#include "libpdbg.h"

static pdbg_progress_tick_t progress_tick;

struct pdbg_target *__pdbg_next_target(const char *class, struct pdbg_target *parent, struct pdbg_target *last)
{
	struct pdbg_target *next, *tmp;
	struct pdbg_target_class *target_class;

	if (class && !find_target_class(class))
		return NULL;

	target_class = require_target_class(class);

retry:
	/* No more targets left to check in this class */
	if ((last && last->class_link.next == &target_class->targets.n) ||
	    list_empty(&target_class->targets))
		return NULL;

	if (last)
		next = list_entry(last->class_link.next, struct pdbg_target, class_link);
	else
		if (!(next = list_top(&target_class->targets, struct pdbg_target, class_link)))
			return NULL;

	if (!parent)
		/* Parent is null, no need to check if this is a child */
		return next;
	else {
		/* Check if this target is a child of the given parent */
		for (tmp = next; tmp && next->parent && tmp != parent; tmp = tmp->parent) {}
		if (tmp == parent)
			return next;
		else {
			last = next;
			goto retry;
		}
	}
}

struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last)
{
	if (!parent || list_empty(&parent->children))
		return NULL;

	if (!last)
		return list_top(&parent->children, struct pdbg_target, list);

	if (last->list.next == &parent->children.n)
		return NULL;

	return list_entry(last->list.next, struct pdbg_target, list);
}

enum pdbg_target_status pdbg_target_status(struct pdbg_target *target)
{
	return target->status;
}

void pdbg_target_status_set(struct pdbg_target *target, enum pdbg_target_status status)
{
	/* It's a programming error for user code to attempt anything else so
	 * blow up obviously if this happens */
	assert(status == PDBG_TARGET_DISABLED || status == PDBG_TARGET_MUSTEXIST);

	target->status = status;
}

/* Searches up the tree and returns the first valid index found */
uint32_t pdbg_target_index(struct pdbg_target *target)
{
	struct pdbg_target *dn;

	for (dn = target; dn && dn->index == -1; dn = dn->parent);

	if (!dn)
		return -1;
	else
		return dn->index;
}

/* Find a target parent from the given class */
struct pdbg_target *pdbg_target_parent(const char *class, struct pdbg_target *target)
{
	struct pdbg_target *parent;

	if (!class)
		return target->parent;

	for (parent = target->parent; parent && parent->parent; parent = parent->parent) {
		if (!strcmp(class, pdbg_target_class_name(parent)))
			return parent;
	}

	return NULL;
}

struct pdbg_target *pdbg_target_require_parent(const char *class, struct pdbg_target *target)
{
	struct pdbg_target *parent = pdbg_target_parent(class, target);

	assert(parent);
	return parent;
}

/* Searched up the tree for the first target of the right class and returns its index */
uint32_t pdbg_parent_index(struct pdbg_target *target, char *class)
{
	struct pdbg_target *parent;

	parent = pdbg_target_parent(class, target);
	if (parent)
		return pdbg_target_index(parent);
	else
		return -1;
}

char *pdbg_target_class_name(struct pdbg_target *target)
{
	return target->class;
}

char *pdbg_target_name(struct pdbg_target *target)
{
	return target->name;
}

const char *pdbg_target_dn_name(struct pdbg_target *target)
{
	return target->dn_name;
}

int pdbg_target_u32_property(struct pdbg_target *target, const char *name, uint32_t *val)
{
	uint32_t *p;
	size_t size;

	p = pdbg_get_target_property(target, name, &size);
	if (!p)
		return -1;

	assert(size == 4);
	*val = be32toh(*p);

	return 0;
}

uint32_t pdbg_target_chip_id(struct pdbg_target *target)
{
        uint32_t id;

	while (pdbg_target_u32_property(target, "chip-id", &id)) {
		target = target->parent;

		/* If we hit this we've reached the top of the tree
		 * and haven't found chip-id */
		assert(target);
	}

	return id;
}

void pdbg_progress_tick(uint64_t cur, uint64_t end)
{
	if (progress_tick)
		progress_tick(cur, end);
}

void pdbg_set_progress_tick(pdbg_progress_tick_t fn)
{
	progress_tick = fn;
}
