/* Copyright 2018 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __LIBPDBG_CHIP_H
#define __LIBPDBG_CHIP_H

uint64_t mfspr(uint64_t reg, uint64_t spr) __attribute__ ((visibility("hidden")));
uint64_t mtspr(uint64_t spr, uint64_t reg) __attribute__ ((visibility("hidden")));
int ram_instructions(struct pdbg_target *thread_target, uint64_t *opcodes,
		uint64_t *results, int len, unsigned int lpar) __attribute__
		((visibility("hidden")));
#endif
