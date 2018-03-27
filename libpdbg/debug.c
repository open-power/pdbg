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
#include <stdarg.h>

#include "debug.h"

static int pdbg_loglevel = PDBG_ERROR;

static void pdbg_logfunc_default(int loglevel, const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
}

static pdbg_log_func_t pdbg_logfunc = pdbg_logfunc_default;

void pdbg_set_logfunc(pdbg_log_func_t fn)
{
	if (fn == NULL)
		return;

	pdbg_logfunc = fn;
}

void pdbg_set_loglevel(int loglevel)
{
	if (loglevel < PDBG_ERROR)
		pdbg_loglevel = PDBG_ERROR;
	else if (loglevel > PDBG_DEBUG)
		pdbg_loglevel = PDBG_DEBUG;
	else
		pdbg_loglevel = loglevel;
}

void pdbg_log(int loglevel, const char *fmt, ...)
{
	va_list ap;

	if (loglevel > pdbg_loglevel)
		return;

	va_start(ap, fmt);
	pdbg_logfunc(loglevel, fmt, ap);
	va_end(ap);
}
