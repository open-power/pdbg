#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <ccan/list/list.h>

#include "target.h"

void target_init(struct target *target, const char *name, uint64_t base,
		 target_read read, target_write write,
		 target_destroy destroy, struct target *next)
{
	target->name = name;
	target->index = -1;
	target->base = base;
	target->read = read;
	target->write = write;
	target->destroy = destroy;
	target->next = next;
	target->status = 0;

	list_head_init(&target->children);

	if (next)
		list_add_tail(&next->children, &target->link);
}

void target_del(struct target *target)
{
	if (target->destroy)
		target->destroy(target);

	/* We don't recursively destroy things yet */
	assert(list_empty(&target->children));
	if (target->next)
		list_del(&target->link);
}

int read_target(struct target *target, uint64_t addr, uint64_t *value)
{
//	printf("Target %s read 0x%0llx\n", target->name, addr);

	if (target->read)
		return target->read(target, addr, value);
	else
		/* If there is no read method fall through to the next one */
		return read_next_target(target, addr, value);
}

int read_next_target(struct target *target, uint64_t addr, uint64_t *value)
{
	assert(target->next);
	return read_target(target->next, target->base + addr, value);
}

int write_target(struct target *target, uint64_t addr, uint64_t value)
{
//	printf("Target %s write 0x%0llx\n", target->name, addr);

	if (target->write)
		return target->write(target, addr, value);
	else
		return write_next_target(target, addr, value);
}

int write_next_target(struct target *target, uint64_t addr, uint64_t value)
{
	assert(target->next);
	return write_target(target->next, target->base + addr, value);
}
