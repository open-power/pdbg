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
#ifndef __UTIL_H
#define __UTIL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Parse a range or a list of numbers from a string into an array
 *
 * For each number present in the string, set the corresponding list element
 * to 1.  The list acts as the index flags.  The range of valid numbers varies
 * from 0 to sizeof(list)-1.
 *
 * @param[in]  arg String to parse
 * @param[in]  max The size of the list
 * @param[in]  list The list of flags
 * @param[out] count Optional count of distinct numbers found
 * @return true on success, false on error
 */
bool parse_list(const char *arg, int max, int *list, int *count);

/**
 * @brief Dump bytes in hex similar to hexdump format
 *
 * Prints 16 bytes per line in the specified groups.  The addresses are
 * printed aligned to 16 bytes.
 *
 * @param[in]  addr Address
 * @param[in]  buf Data to print
 * @param[in]  size Number of bytes to print
 * @param[in]  group_size How to group bytes (valid values 1, 2, 4, 8)
 */
void hexdump(uint64_t addr, uint8_t *buf, uint64_t size, uint8_t group_size);

#endif
