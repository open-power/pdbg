#include <string.h>
#include <endian.h>

#include "target.h"
#include "libpdbg.h"

static pdbg_progress_tick_t progress_tick;
static bool pdbg_short_context = false;

struct pdbg_target *get_parent(struct pdbg_target *target, bool system)
{
	struct pdbg_target *parent;

	if (!target)
		return NULL;

	/*
	 * To find a parent in the system tree:
	 *   - If a target is real, map it to possible virtual target
	 *   - Calculate the parent
	 *   - If the parent is virtual, map it to real target
	 *
	 * To find a parent in the backend tree:
	 *   - Target will be real or virtual without mapped real node
	 *   - Calculate the parent
	 *   - If the parent is virtual, map it to real target
	 */
	if (system)
		target = target_to_virtual(target, false);

	parent = target->parent;

	if (!parent)
		return NULL;

	return target_to_real(parent, false);
}

struct pdbg_target *__pdbg_next_target(const char *class, struct pdbg_target *parent, struct pdbg_target *last, bool system)
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
		for (tmp = next; tmp && get_parent(tmp, system) && tmp != parent; tmp = get_parent(tmp, system)) {}
		if (tmp == parent)
			return next;
		else {
			last = next;
			goto retry;
		}
	}
}

static struct pdbg_target *target_map_child(struct pdbg_target *next, bool system)
{
	/*
	 * Map a target in system tree:
	 *
	 * - If there is no virtual node assiociated, then return the target
	 *   (the target itself can be virtual or real)
	 *
	 * - If there is virtual node associated,
	 *	 - If the target is virtual, return the associated node
	 *	 - If the target is real, return NULL
	 *	   (this target is already covered by previous condition)
	 *
	 * Map a target in backend tree:
	 *
	 *  - If the target is real, return the target
	 *
	 *  - If the target is virtual, return NULL
	 *	(no virtual nodes in backend tree)
	 */
	if (system) {
		if (!next->vnode)
			return next;
		else
			if (target_is_virtual(next))
				return next->vnode;
	} else {
		if (!target_is_virtual(next))
			return next;
	}

	return NULL;
}

struct pdbg_target *__pdbg_next_child_target(struct pdbg_target *parent, struct pdbg_target *last, bool system)
{
	struct pdbg_target *next, *child = NULL;

	if (!parent)
		return NULL;

	/*
	 * Parent node can be virtual or real.
	 *
	 * If the parent node doesn't have any children,
	 *	- If there is associated virtual node,
	 *		Use that node as parent
	 *
	 *	- If there is no associated virtual node,
	 *		No children
	 */
	if (list_empty(&parent->children)) {
		if (parent->vnode)
			parent = parent->vnode;
		else
			return NULL;
	}

	 /*
	 * If the parent node has children,
	 *	- Traverse the children
	 *	   (map the children in system or backend tree)
	 */
	if (!last) {
		list_for_each(&parent->children, child, list)
			if ((next = target_map_child(child, system)))
				return next;

		return NULL;
	}

	/*
	 * In a system tree traversal, virtual targets with associated
	 * nodes, get mapped to real targets.
	 *
	 * When the last child is specified:
	 *   - If in a system tree traverse, and
	 *	 the last child has associated node, and
	 *	 the last child is real
	 *		Then the last child is not the actual child of the parent
	 *		  (the associated virtual node is the actual child)
	 */
	if (system)
		last = target_to_virtual(last, false);

	if (last == list_tail(&parent->children, struct pdbg_target, list))
		return NULL;

	child = last;
	do {
		child = list_entry(child->list.next, struct pdbg_target, list);
		if ((next = target_map_child(child, system)))
			return next;
	} while (child != list_tail(&parent->children, struct pdbg_target, list));

	return NULL;
}

static int _pdbg_target_traverse(struct pdbg_target *parent,
				 pdbg_target_traverse_callback_fn func,
				 void *priv)
{
	struct pdbg_target *target;
	int ret;

	ret = func(parent, priv);
	if (ret)
		return ret;

	pdbg_for_each_child_target(parent, target) {
		ret = _pdbg_target_traverse(target, func, priv);
		if (ret)
			return ret;
	}

	return 0;
}

int pdbg_target_traverse(struct pdbg_target *parent,
			 pdbg_target_traverse_callback_fn func,
			 void *priv)
{
	if (!parent)
		parent = pdbg_target_root();

	return _pdbg_target_traverse(parent, func,  priv);
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

	for (dn = target; dn && dn->index == -1; dn = get_parent(dn, true));

	if (!dn)
		return -1;
	else
		return dn->index;
}

/* Find a target parent from the given class */
struct pdbg_target *target_parent(const char *klass, struct pdbg_target *target, bool system)
{
	struct pdbg_target *parent;

	if (!klass)
		return get_parent(target, system);

	for (parent = get_parent(target, system); parent && get_parent(parent, system); parent = get_parent(parent, system)) {
		const char *tclass = pdbg_target_class_name(parent);

		if (!tclass)
			continue;

		if (!strcmp(klass, tclass))
			return parent;
	}

	return NULL;
}

struct pdbg_target *pdbg_target_parent(const char *klass, struct pdbg_target *target)
{
	return target_parent(klass, target, true);
}

struct pdbg_target *pdbg_target_parent_virtual(const char *klass, struct pdbg_target *target)
{
	return target_parent(klass, target, false);
}

struct pdbg_target *require_target_parent(const char *klass, struct pdbg_target *target, bool system)
{
	struct pdbg_target *parent = target_parent(klass, target, system);

	assert(parent);
	return parent;
}

struct pdbg_target *pdbg_target_require_parent(const char *klass, struct pdbg_target *target)
{
	return require_target_parent(klass, target, true);
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

const char *pdbg_target_class_name(struct pdbg_target *target)
{
	return target->class;
}

const char *pdbg_target_name(struct pdbg_target *target)
{
	return target->name;
}

const char *pdbg_target_dn_name(struct pdbg_target *target)
{
	return target->dn_name;
}

int pdbg_target_u32_property(struct pdbg_target *target, const char *name, uint32_t *val)
{
	const void *p;
	size_t size;

	p = pdbg_get_target_property(target, name, &size);
	if (!p)
		return -1;

	assert(size == 4);
	*val = be32toh(*(uint32_t *)p);

	return 0;
}

int pdbg_target_u32_index(struct pdbg_target *target, const char *name, int index, uint32_t *val)
{
	const void *p;
		size_t len;

		p = pdbg_get_target_property(target, name, &len);
		if (!p)
				return -1;

		assert(len >= (index+1)*sizeof(uint32_t));

	/* FDT pointers should be aligned, but best to check */
	assert(!((uintptr_t) p & 0x3));

		/* Always aligned, so this works. */
		*val = be32toh(((uint32_t *)p)[index]);
		return 0;
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

bool pdbg_context_short(void)
{
	if (pdbg_target_root()) {
		pdbg_log(PDBG_ERROR, "pdbg_context_short() must be called before pdbg_targets_init()\n");
		return false;
	}

	pdbg_short_context = true;
	return true;
}

bool pdbg_context_is_short(void)
{
	return pdbg_short_context;
}

struct pdbg_target *pdbg_get_unattached_backend_target(struct pdbg_target* target, const char* class_name)
{
	if (!target || !class_name)
	{
		pdbg_log(PDBG_ERROR, "Invalid input: target or class_name is NULL\n");
		return NULL;
	}
	
	struct pdbg_target* backend_target = NULL;

	/* We assume each processor chip contains one and only one pib */
	pdbg_for_each_target(class_name, target, backend_target)
	{
		/* There is a vnode attached, so this should be the backend target*/
		if(!backend_target->vnode)
		{
			break;
		}
	}
	return backend_target;
}

struct pdbg_target *pdbg_get_backend_target(struct pdbg_target* target, const char* class_name)
{
	if (!target || !class_name)
	{
		pdbg_log(PDBG_ERROR, "Invalid input: target or class_name is NULL\n");
		return NULL;
	}

	struct pdbg_target* backend_target = NULL;
	/* We assume each processor chip contains one and only one pib */
	pdbg_for_each_target(class_name, target, backend_target)
	{
		/* There is a vnode attached, so this should be the current backend target*/
		if(backend_target->vnode)
		{
			break;
		}
	}

	/* If the pib target is not a child target, look for the parent pib target
	But the translation does not happen here. So better to */
	if (!backend_target)
	{
		backend_target = target_parent(class_name, target, false);
		if (!backend_target)
		{
			pdbg_log(PDBG_ERROR, "unable to get %s target for %s\n",
				class_name, pdbg_target_path(target));
			return NULL;
		}
	}
	return backend_target;
}

struct pdbg_target* pdbg_switch_node_backend(struct pdbg_target* target, char* backend)
{
	struct pdbg_target* attached_pib_target = pdbg_get_backend_target(target,"pib");
	if (!attached_pib_target)
	{
		pdbg_log(PDBG_ERROR, "Failed to get attached PIB target\n");
		return NULL;
	}
	
	if (backend != NULL && backend[0] != '\0') 
	{
		const char *cur_backend  = pdbg_target_property(attached_pib_target, "backend", NULL);
		if(strcmp(cur_backend, backend) == 0)
		{
			//The current backend is same as expected backend. return
			return attached_pib_target;
		}
	}

	struct pdbg_target* virtual_target = target_to_virtual(attached_pib_target, true);
	struct pdbg_target* new_pib_target = pdbg_get_unattached_backend_target(target,"pib");
	
	if(virtual_target && new_pib_target)
	{
		dt_link_virtual(virtual_target, new_pib_target);
		/*remove the virtual link as we have switched the backend*/
		attached_pib_target->vnode = NULL;
		return new_pib_target;
	}
	return NULL;
}

