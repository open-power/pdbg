/* Copyright 2018 IBM Corp.
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
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>

#include <libpdbg.h>

#include "path.h"
#include "util.h"

#define MAX_PATH_COMP_LEN	32
#define MAX_PATH_COMPONENTS	16
#define MAX_PATH_LEN		(MAX_PATH_COMP_LEN * MAX_PATH_COMPONENTS)

/* This is max(MAX_PROCESSORS, MAX_CHIPS, MAX_THREADS) */
#define MAX_PATH_INDEX		64

struct path_pattern {
	char prefix[MAX_PATH_COMP_LEN];
	int index[MAX_PATH_INDEX];
	bool match_full;
	bool match_index;
};

/* Start with max threads, rather than defining arbitrary number */
#define MAX_TARGETS	(64 * 24 * 8)

static struct pdbg_target *path_target[MAX_TARGETS];
static unsigned int path_target_count;

static void safe_strcpy(char *dest, size_t n, const char *src)
{
	assert(strlen(src) + 1 <= n);

	strcpy(dest, src);
}

/*
 * Parse string components of following forms:
 *	pib0
 *	core[1-3,11-13]
 *	thread*
 *	adu@123000
 */
static bool path_pattern_parse(const char *arg, struct path_pattern *pat)
{
	char tmp[strlen(arg)+1];
	char *tok;
	bool ok;

	safe_strcpy(tmp, sizeof(tmp), arg);

	memset(pat, 0, sizeof(*pat));

	if (strchr(tmp, '@')) {
		safe_strcpy(pat->prefix, sizeof(pat->prefix), tmp);
		pat->match_full = true;

	} else if (strchr(tmp, '*')) {
		tok = strtok(tmp, "*");
		if (!tok)
			safe_strcpy(pat->prefix, sizeof(pat->prefix), "all");
		else
			safe_strcpy(pat->prefix, sizeof(pat->prefix), tok);

	} else if (strchr(tmp, '[')) {
		tok = strtok(tmp, "[");
		if (tok == NULL) {
			fprintf(stderr, "Invalid pattern '%s'\n", arg);
			return false;
		}
		safe_strcpy(pat->prefix, sizeof(pat->prefix), tok);

		tok = strtok(NULL, "]");
		if (!tok) {
			fprintf(stderr, "Invalid pattern '%s'\n", arg);
			return false;
		}

		ok = parse_list(tok, MAX_PATH_INDEX, pat->index, NULL);
		if (!ok)
			return false;

		pat->match_index = true;

	} else {
		size_t n = strlen(tmp) - 1;
		while (n >= 0 && isdigit(tmp[n]))
			n--;
		n++;

		if (n != strlen(tmp)) {
			int index;

			index = atoi(&tmp[n]);
			if (index < 0 || index >= MAX_PATH_INDEX) {
				fprintf(stderr, "Invalid index '%s'\n", &tmp[n]);
				return false;
			}
			tmp[n] = '\0';
			pat->index[index] = 1;
			pat->match_index = true;
		}

		safe_strcpy(pat->prefix, sizeof(pat->prefix), tmp);
	}

	if (!pat->prefix)
		return false;

	return true;
}

static int path_pattern_split(const char *arg, struct path_pattern *pats)
{
	char arg_copy[MAX_PATH_LEN];
	char *tmp, *tok, *saveptr = NULL;
	int n;
	bool ok;

	safe_strcpy(arg_copy, sizeof(arg_copy), arg);

	tmp = arg_copy;
	n = 0;
	while ((tok = strtok_r(tmp, "/", &saveptr))) {
		size_t len = strlen(tok);

		if (len == 0)
			continue;

		if (len >= MAX_PATH_LEN) {
			fprintf(stderr, "Pattern component '%s' too long\n", tok);
			return -1;
		}

		ok = path_pattern_parse(tok, &pats[n]);
		if (!ok)
			return -1;

		n++;
		assert(n <= MAX_PATH_COMPONENTS);

		tmp = NULL;
	}

	return n;
}

static int path_target_find(struct pdbg_target *prev)
{
	int i;

	if (!prev)
		return -1;

	for (i=0; i<path_target_count; i++) {
		if (path_target[i] == prev)
			return i;
	}

	return -1;
}

struct pdbg_target *path_target_find_next(const char *klass, int index)
{
	struct pdbg_target *target;
	int i;

	for (i=index+1; i<path_target_count; i++) {
		target = path_target[i];
		if (klass) {
			const char *classname = pdbg_target_class_name(target);

			if (!strcmp(klass, classname))
				return target;
		} else {
			return target;
		}
	}

	return NULL;
}

bool path_target_add(struct pdbg_target *target)
{
	int index;

	index = path_target_find(target);
	if (index >= 0)
		return true;

	if (path_target_count == MAX_TARGETS)
		return false;

	path_target[path_target_count] = target;
	path_target_count++;
	return true;
}

static void path_pattern_match(struct pdbg_target *target,
			       struct path_pattern *pats,
			       int max_levels,
			       int level)
{
	struct pdbg_target *child;
	char comp_name[MAX_PATH_COMP_LEN];
	const char *classname;
	char *tok;
	int next = level;
	bool found = false;

	if (target == pdbg_target_root()) {
		goto end;
	}

	if (!strcmp("all", pats[level].prefix)) {
		if (!path_target_add(target))
			return;
		goto end;
	}

	classname = pdbg_target_class_name(target);
	if (!classname)
		goto end;

	if (pats[level].match_full) {
		const char *dn_name = pdbg_target_dn_name(target);

		safe_strcpy(comp_name, sizeof(comp_name), dn_name);
	} else {
		safe_strcpy(comp_name, sizeof(comp_name), classname);
	}
	tok = comp_name;

	if (!strcmp(tok, pats[level].prefix)) {
		found = true;

		if (pats[level].match_index) {
			int index = pdbg_target_index(target);

			if (pats[level].index[index] != 1)
				found = false;
		}
	}

	if (found) {
		if (level == max_levels-1) {
			path_target_add(target);
			return;
		} else {
			next = level + 1;
		}
	}

end:
	pdbg_for_each_child_target(target, child) {
		path_pattern_match(child, pats, max_levels, next);
	}
}

bool path_target_parse(const char **arg, int arg_count)
{
	struct path_pattern pats[MAX_PATH_COMPONENTS];
	int i, n;

	for (i=0; i<arg_count; i++) {
		n = path_pattern_split(arg[i], pats);
		if (n <= 0)
			return false;

		path_pattern_match(pdbg_target_root(), pats, n, 0);
	}

	return true;
}

bool path_target_present(void)
{
	return (path_target_count > 0);
}

bool path_target_selected(struct pdbg_target *target)
{
	int index;

	index = path_target_find(target);
	if (index >= 0)
		return true;

	return false;
}

void path_target_dump(void)
{
	struct pdbg_target *target;
	char *path;

	for_each_path_target(target) {
		path = pdbg_target_path(target);
		assert(path);
		printf("%s\n", path);
		free(path);
	}
}

struct pdbg_target *path_target_next(struct pdbg_target *prev)
{
	int index;

	index = path_target_find(prev);
	return path_target_find_next(NULL, index);
}

struct pdbg_target *path_target_next_class(const char *klass,
					   struct pdbg_target *prev)
{
	int index;

	index = path_target_find(prev);
	return path_target_find_next(klass, index);
}
