/* Copyright 2017 IBM Corp.
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
#include "main.h"

enum backend default_backend(void)
{
	return FAKE;
}

bool backend_is_possible(enum backend backend)
{
	return backend == FAKE;
}

void print_backends(FILE *stream)
{
	fprintf(stream, "FAKE");
}

/* Theres no target for FAKE backend */
const char *default_target(enum backend backend)
{
	return NULL;
}

void print_targets(FILE *stream)
{
	fprintf(stream, "FAKE: No target is necessary\n");
}

/* Theres no device for FAKE backend */
bool target_is_possible(enum backend backend, const char *target)
{
	return target == NULL;
}
