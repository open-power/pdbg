/* Copyright 2013-2014 IBM Corp.
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
#include <stdarg.h>
#include "target.h"
#include <libfdt/libfdt.h>
#include <libfdt/libfdt_internal.h>
#include <ccan/list/list.h>
#include <ccan/short_types/short_types.h>
#include <ccan/str/str.h>
#include <endian.h>

#include "debug.h"
#include "compiler.h"
#include "hwunit.h"

#define prerror printf
#define is_rodata(p) false

#define dt_for_each_child(parent, node) \
	list_for_each(&parent->children, node, list)

/* Used to give unique handles. */
static uint32_t last_phandle = 0;

static struct pdbg_target *pdbg_dt_root;

static const char *take_name(const char *name)
{
	if (!is_rodata(name) && !(name = strdup(name))) {
		prerror("Failed to allocate copy of name");
		abort();
	}
	return name;
}

/* Adds information representing an actual target */
static struct pdbg_target *dt_pdbg_target_new(const void *fdt, int node_offset)
{
	struct pdbg_target *target;
	struct pdbg_target_class *target_class;
	const struct hw_unit_info *hw_info = NULL;
	const struct fdt_property *prop;
	size_t size;

	prop = fdt_get_property(fdt, node_offset, "compatible", NULL);
	if (prop) {
		int i, prop_len = fdt32_to_cpu(prop->len);

		/*
		 * If I understand correctly, the property we have
		 * here can be a stringlist with a few compatible
		 * strings
		 */
		i = 0;
		while (i < prop_len) {
			hw_info = pdbg_hwunit_find_compatible(&prop->data[i]);
			if (hw_info) {
				size = hw_info->size;
				break;
			}

			i += strlen(&prop->data[i]) + 1;
		}
	}

	if (!hw_info)
		/* Couldn't find anything implementing this target */
		return NULL;

	target = calloc(1, size);
	if (!target) {
		prerror("Failed to allocate node\n");
		abort();
	}

	/* hw_info->hw_unit points to a per-target struct type. This
	 * works because the first member in the per-target struct is
	 * guaranteed to be the struct pdbg_target (see the comment
	 * above DECLARE_HW_UNIT). */
	memcpy(target, hw_info->hw_unit, size);
	target_class = get_target_class(target);
	list_add_tail(&target_class->targets, &target->class_link);

	return target;
}

static struct pdbg_target *dt_new_node(const char *name, void *fdt, int node_offset)
{
	struct pdbg_target *node = NULL;
	size_t size = sizeof(*node);

	if (fdt)
		node = dt_pdbg_target_new(fdt, node_offset);

	if (!node)
		node = calloc(1, size);

	if (!node) {
		prerror("Failed to allocate node\n");
		abort();
	}

	node->fdt = fdt;
	node->fdt_offset = fdt ? node_offset : -1;

	node->dn_name = take_name(name);
	node->parent = NULL;
	list_head_init(&node->properties);
	list_head_init(&node->children);
	node->phandle = ++last_phandle;

	return node;
}

static const char *get_unitname(const struct pdbg_target *node)
{
	const char *c = strchr(node->dn_name, '@');

	if (!c)
		return NULL;

	return c + 1;
}

static int dt_cmp_subnodes(const struct pdbg_target *a, const struct pdbg_target *b)
{
	const char *a_unit = get_unitname(a);
	const char *b_unit = get_unitname(b);

	ptrdiff_t basenamelen = a_unit - a->dn_name;

	/* sort hex unit addresses by number */
	if (a_unit && b_unit && !strncmp(a->dn_name, b->dn_name, basenamelen)) {
		unsigned long long a_num, b_num;
		char *a_end, *b_end;

		a_num = strtoul(a_unit, &a_end, 16);
		b_num = strtoul(b_unit, &b_end, 16);

		/* only compare if the unit addr parsed correctly */
		if (*a_end == 0 && *b_end == 0)
			return (a_num > b_num) - (a_num < b_num);
	}

	return strcmp(a->dn_name, b->dn_name);
}

static bool dt_attach_node(struct pdbg_target *parent, struct pdbg_target *child)
{
	struct pdbg_target *node = NULL;

	assert(!child->parent);

	if (list_empty(&parent->children)) {
		list_add(&parent->children, &child->list);
		child->parent = parent;

		return true;
	}

	dt_for_each_child(parent, node) {
		int cmp = dt_cmp_subnodes(node, child);

		/* Look for duplicates */
		if (cmp == 0) {
			prerror("DT: %s failed, duplicate %s\n",
				__func__, child->dn_name);
			return false;
		}

		/* insert before the first node that's larger
		 * the the node we're inserting */
		if (cmp > 0)
			break;
	}

	list_add_before(&parent->children, &child->list, &node->list);
	child->parent = parent;

	return true;
}

static char *dt_get_path(struct pdbg_target *node)
{
	unsigned int len = 0;
	struct pdbg_target *n;
	char *path, *p;

	/* Dealing with NULL is for test/debug purposes */
	if (!node)
		return strdup("<NULL>");

	for (n = node; n; n = n->parent) {
		n = target_to_virtual(n, false);
		len += strlen(n->dn_name);
		if (n->parent || n == node)
			len++;
	}
	path = calloc(1, len + 1);
	assert(path);
	p = path + len;
	for (n = node; n; n = n->parent) {
		n = target_to_virtual(n, false);
		len = strlen(n->dn_name);
		p -= len;
		memcpy(p, n->dn_name, len);
		if (n->parent || n == node)
			*(--p) = '/';
	}
	assert(p == path);

	return p;
}

static const char *__dt_path_split(const char *p,
				   const char **namep, unsigned int *namel,
				   const char **addrp, unsigned int *addrl)
{
	const char *at, *sl;

	*namel = *addrl = 0;

	/* Skip initial '/' */
	while (*p == '/')
		p++;

	/* Check empty path */
	if (*p == 0)
		return p;

	at = strchr(p, '@');
	sl = strchr(p, '/');
	if (sl == NULL)
		sl = p + strlen(p);
	if (sl < at)
		at = NULL;
	if (at) {
		*addrp = at + 1;
		*addrl = sl - at - 1;
	}
	*namep = p;
	*namel = at ? (at - p) : (sl - p);

	return sl;
}

static struct pdbg_target *dt_find_by_path(struct pdbg_target *root, const char *path)
{
	struct pdbg_target *n;
	const char *pn, *pa = NULL, *p = path, *nn = NULL, *na = NULL;
	unsigned int pnl, pal, nnl, nal;
	bool match, vnode;

	/* Walk path components */
	while (*p) {
		/* Extract next path component */
		p = __dt_path_split(p, &pn, &pnl, &pa, &pal);
		if (pnl == 0 && pal == 0)
			break;

		vnode = false;

again:
		/* Compare with each child node */
		match = false;
		list_for_each(&root->children, n, list) {
			match = true;
			__dt_path_split(n->dn_name, &nn, &nnl, &na, &nal);
			if (pnl && (pnl != nnl || strncmp(pn, nn, pnl)))
				match = false;
			if (pal && (pal != nal || strncmp(pa, na, pal)))
				match = false;
			if (match) {
				root = n;
				break;
			}
		}

		/* No child match */
		if (!match) {
			if (!vnode && root->vnode) {
				vnode = true;
				root = root->vnode;
				goto again;
			}
			return NULL;
		}
	}
	return target_to_real(root, false);
}

static void dt_add_phandle(struct pdbg_target *node, const char *name,
			   const void *val, size_t size)
{
	/*
	 * Filter out phandle properties, we re-generate them
	 * when flattening
	 */
	if (strcmp(name, "linux,phandle") == 0 ||
	    strcmp(name, "phandle") == 0) {
		assert(size == 4);
		node->phandle = *(const u32 *)val;
		if (node->phandle >= last_phandle)
			last_phandle = node->phandle;
	}
}

static const void *_target_property(struct pdbg_target *target, const char *name, size_t *size)
{
	const void *buf;
	int buflen;

	if (target->fdt_offset == -1) {
		size ? *size = 0 : 0;
		return NULL;
	}

	buf = fdt_getprop(target->fdt, target->fdt_offset, name, &buflen);
	if (!buf) {
		size ? *size = 0 : 0;
		return NULL;
	}

	size ? *size = buflen : 0;
	return buf;
}

bool pdbg_target_set_property(struct pdbg_target *target, const char *name, const void *val, size_t size)
{
	const void *p;
	size_t len;
	int ret;

	p = _target_property(target, name, &len);
	if (!p && target->vnode) {
		target = target->vnode;
		p = _target_property(target, name, &len);
	}
	if (!p)
		return false;

	if (!target->fdt || !pdbg_fdt_is_writeable(target->fdt))
		return false;

	if (len != size)
		return false;

	ret = fdt_setprop_inplace(target->fdt, target->fdt_offset, name, val, size);
	if (ret)
		return false;

	return true;
}

const void *pdbg_target_property(struct pdbg_target *target, const char *name, size_t *size)
{
	const void *p;

	p = _target_property(target, name, size);
	if (!p && target->vnode)
		p = _target_property(target->vnode, name, size);

	return p;
}

static u32 dt_property_get_cell(const void *prop, size_t len, u32 index)
{
	assert(len >= (index+1)*sizeof(u32));
	/* Always aligned, so this works. */
	return fdt32_to_cpu(((const u32 *)prop)[index]);
}

/* First child of this node. */
static struct pdbg_target *dt_first(const struct pdbg_target *root)
{
	return list_top(&root->children, struct pdbg_target, list);
}

/* Return next node, or NULL. */
static struct pdbg_target *dt_next(const struct pdbg_target *root,
			const struct pdbg_target *prev)
{
	/* Children? */
	if (!list_empty(&prev->children))
		return dt_first(prev);

	do {
		/* More siblings? */
		if (prev->list.next != &prev->parent->children.n)
			return list_entry(prev->list.next, struct pdbg_target,list);

		/* No more siblings, move up to parent. */
		prev = prev->parent;
	} while (prev != root);

	return NULL;
}

static const void *dt_require_property(struct pdbg_target *node,
				       const char *name, int wanted_len,
				       size_t *prop_len)
{
	const void *p;
	size_t len;

	p = pdbg_target_property(node, name, &len);
	if (!p) {
		const char *path = dt_get_path(node);

		prerror("DT: Missing required property %s/%s\n",
			path, name);
		assert(false);
	}
	if (wanted_len >= 0 && len != wanted_len) {
		const char *path = dt_get_path(node);

		prerror("DT: Unexpected property length %s/%s\n",
			path, name);
		prerror("DT: Expected len: %d got len: %zu\n",
			wanted_len, len);
		assert(false);
	}

	if (prop_len) {
		*prop_len = len;
	}
	return p;
}

bool pdbg_target_compatible(struct pdbg_target *target, const char *compatible)
{
	const char *c, *end;
        size_t len;

        c = (const char *)pdbg_target_property(target, "compatible", &len);
        if (!c)
                return false;

        end = c + len;
        while(c < end) {
                if (!strcasecmp(compatible, c))
                        return true;

                c += strlen(c) + 1;
        }

        return false;
}

struct pdbg_target *__pdbg_next_compatible_node(struct pdbg_target *root,
                                                struct pdbg_target *prev,
                                                const char *compat)
{
        struct pdbg_target *target;

        target = prev ? dt_next(root, prev) : root;
        for (; target; target = dt_next(root, target))
                if (pdbg_target_compatible(target, compat))
                        return target;
        return NULL;
}

static uint32_t dt_prop_get_u32_def(struct pdbg_target *node,
				    const char *prop, uint32_t def)
{
	const void *p;
	size_t len;

	p = pdbg_target_property(node, prop, &len);
        if (!p)
                return def;


        return dt_property_get_cell(p, len, 0);
}

static enum pdbg_target_status str_to_status(const char *status)
{
	if (!strcmp(status, "enabled")) {
		/* There isn't really a use case for this at the moment except
		 * perhaps as some kind of expert mode which bypasses the usual
		 * probing process. However simply marking a top-level node as
		 * enabled would not be enough to make parent nodes enabled so
		 * this wouldn't work as expected anyway. */
		assert(0);
		return PDBG_TARGET_ENABLED;
	} else if (!strcmp(status, "disabled"))
		return PDBG_TARGET_DISABLED;
	else if (!strcmp(status, "mustexist"))
		return PDBG_TARGET_MUSTEXIST;
	else if (!strcmp(status, "nonexistent"))
		return PDBG_TARGET_NONEXISTENT;
	else if (!strcmp(status, "unknown"))
		return PDBG_TARGET_UNKNOWN;
	else
		assert(0);
}

static int dt_expand_node(struct pdbg_target *node, void *fdt, int fdt_node)
{
	const struct fdt_property *prop;
	int offset, nextoffset, err;
	struct pdbg_target *child;
	const char *name;
	uint32_t tag;
	uint32_t data;

	if (((err = fdt_check_header(fdt)) != 0)
	    || (fdt_node < 0) || (fdt_node % FDT_TAGSIZE)
	    || (fdt_next_tag(fdt, fdt_node, &fdt_node) != FDT_BEGIN_NODE)) {
		prerror("FDT: Error %d parsing node 0x%x\n", err, fdt_node);
		return -1;
	}

	nextoffset = fdt_node;
	do {
		offset = nextoffset;

		tag = fdt_next_tag(fdt, offset, &nextoffset);
		switch (tag) {
		case FDT_PROP:
			prop = fdt_offset_ptr(fdt, offset, 0);
			name = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));
			if (strcmp("index", name) == 0) {
				memcpy(&data, prop->data, sizeof(data));
				node->index = fdt32_to_cpu(data);
			}

			if (strcmp("status", name) == 0)
				node->status = str_to_status(prop->data);

			dt_add_phandle(node, name, prop->data,
					fdt32_to_cpu(prop->len));
			break;
		case FDT_BEGIN_NODE:
			name = fdt_get_name(fdt, offset, NULL);
			child = dt_new_node(name, fdt, offset);
			assert(child);
			nextoffset = dt_expand_node(child, fdt, offset);

			/*
			 * This may fail in case of duplicate, keep it
			 * going for now, we may ultimately want to
			 * assert
			 */
			(void)dt_attach_node(node, child);
			break;
		case FDT_END:
			return -1;
		}
	} while (tag != FDT_END_NODE);

	return nextoffset;
}

static void dt_expand(struct pdbg_target *root, void *fdt)
{
	PR_DEBUG("FDT: Parsing fdt @%p\n", fdt);

	if (dt_expand_node(root, fdt, 0) < 0)
		abort();
}

static u64 dt_get_number(const void *pdata, unsigned int cells)
{
	const u32 *p = pdata;
	u64 ret = 0;

	while(cells--)
		ret = (ret << 32) | be32toh(*(p++));
	return ret;
}

static u32 dt_n_address_cells(const struct pdbg_target *node)
{
	if (!node->parent)
		return 0;
	return dt_prop_get_u32_def(node->parent, "#address-cells", 2);
}

static u32 dt_n_size_cells(const struct pdbg_target *node)
{
	if (!node->parent)
		return 0;
	return dt_prop_get_u32_def(node->parent, "#size-cells", 1);
}

uint64_t pdbg_target_address(struct pdbg_target *target, uint64_t *out_size)
{
	const void *p;
	size_t len;

	u32 na = dt_n_address_cells(target);
	u32 ns = dt_n_size_cells(target);
	u32 n;

	p = dt_require_property(target, "reg", -1, &len);
	n = (na + ns) * sizeof(u32);
	assert(n <= len);
	if (out_size)
		*out_size = dt_get_number(p + na * sizeof(u32), ns);
	return dt_get_number(p, na);
}

static struct pdbg_target *dt_new_virtual(struct pdbg_target *root, const char *system_path)
{
	char *parent_path, *sep;
	struct pdbg_target *parent, *vnode;

	parent_path = strdup(system_path);
	assert(parent_path);

	sep = strrchr(parent_path, '/');
	if (!sep || sep[1] == '\0') {
		PR_ERROR("Invalid path reference \"%s\"\n", system_path);
		free(parent_path);
		return NULL;
	}

	*sep = '\0';

	parent = dt_find_by_path(root, parent_path);
	if (!parent) {
		PR_ERROR("Invalid path reference \"%s\"\n", system_path);
		free(parent_path);
		return NULL;
	}

	vnode = dt_new_node(sep+1, NULL, 0);
	assert(vnode);

	free(parent_path);

	if (!dt_attach_node(parent, vnode)) {
		free(vnode);
		return NULL;
	}

	PR_DEBUG("Created virtual node %s\n", system_path);
	return vnode;
}

static void dt_link_virtual(struct pdbg_target *node, struct pdbg_target *vnode)
{
	node->vnode = vnode;
	vnode->vnode = node;
}

static void pdbg_targets_init_virtual(struct pdbg_target *node, struct pdbg_target *root)
{
	struct pdbg_target *vnode, *child = NULL;
	const char *system_path;
	size_t len;

	system_path = (const char *)pdbg_target_property(node, "system-path", &len);
	if (!system_path)
		goto skip;

	assert(!target_is_virtual(node));

	/*
	 * A virtual node identifies the attachment point of a node in the
	 * system tree.
	 */
	vnode = dt_find_by_path(root, system_path);
	if (!vnode)
		vnode = dt_new_virtual(root, system_path);

	/* If virtual node does not exist, or cannot be created, skip */
	if (!vnode)
		goto skip;

	assert(target_is_virtual(vnode));

	/*
	 * If virtual node is not linked, then link with node;
	 * otherwise skip
	 */
	if (!vnode->vnode)
		dt_link_virtual(node, vnode);

skip:
	list_for_each(&node->children, child, list) {
		/* If a virtual node is already linked, skip */
		if (target_is_virtual(child) && child->vnode)
			continue;

		pdbg_targets_init_virtual(child, root);
	}
}

bool pdbg_targets_init(void *fdt)
{
	struct pdbg_dtb *dtb;

	dtb = pdbg_default_dtb(fdt);

	if (!dtb) {
		pdbg_log(PDBG_ERROR, "Could not determine system\n");
		return false;
	}

	if (!dtb->system.fdt) {
		pdbg_log(PDBG_ERROR, "Could not find a system device tree\n");
		return false;
	}

	/* Root node needs to be valid when this function returns */
	pdbg_dt_root = dt_new_node("", dtb->system.fdt, 0);
	if (!pdbg_dt_root)
		return false;

	if (dtb->backend.fdt)
		dt_expand(pdbg_dt_root, dtb->backend.fdt);

	dt_expand(pdbg_dt_root, dtb->system.fdt);

	pdbg_targets_init_virtual(pdbg_dt_root, pdbg_dt_root);
	return true;
}

const char *pdbg_target_path(struct pdbg_target *target)
{
	if (!target->path)
		target->path = dt_get_path(target);

	return target->path;
}

struct pdbg_target *pdbg_target_from_path(struct pdbg_target *target, const char *path)
{
	if (!target)
		target = pdbg_dt_root;

	return dt_find_by_path(target, path);
}

struct pdbg_target *pdbg_target_root(void)
{
	return pdbg_dt_root;
}
