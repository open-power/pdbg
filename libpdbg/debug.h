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
#ifndef __LIBPDBG_DEBUG_H
#define __LIBPDBG_DEBUG_H

#include "libpdbg.h"

void pdbg_log(int log_level, const char* fmt, ...) __attribute__((format (printf, 2, 3)));

#define PR_ERROR(x, args...) \
	pdbg_log(PDBG_ERROR, x, ##args)
#define PR_WARNING(x, args...) \
	pdbg_log(PDBG_WARNING, x, ##args)
#define PR_NOTICE(x, args...) \
	pdbg_log(PDBG_NOTICE, x, ##args)
#define PR_INFO(x, args...) \
	pdbg_log(PDBG_INFO, x, ##args)
#define PR_DEBUG(x, args...) \
	pdbg_log(PDBG_DEBUG, "%s[%d]: " x, __FUNCTION__, __LINE__, ##args)

#endif
