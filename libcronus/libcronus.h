/* Copyright 2019 IBM Corp.
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

#ifndef __LIBCRONUS_H__
#define __LIBCRONUS_H__

#include <stdint.h>

struct cronus_context;

int cronus_connect(const char *hostname, struct cronus_context **out);
void cronus_disconnect(struct cronus_context *cctx);

int cronus_getcfam(struct cronus_context *cctx,
		   int pib_index,
		   uint32_t addr,
		   uint32_t *value);
int cronus_putcfam(struct cronus_context *cctx,
		   int pib_index,
		   uint32_t addr,
		   uint32_t value);

int cronus_getscom(struct cronus_context *cctx,
		   int pib_index,
		   uint64_t addr,
		   uint64_t *value);
int cronus_putscom(struct cronus_context *cctx,
		   int pib_index,
		   uint64_t addr,
		   uint64_t value);

#endif /* __LIBCRONUS_H__ */
