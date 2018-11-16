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

#endif
