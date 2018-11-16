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
#ifndef __PDBG_PATH_H
#define __PDBG_PATH_H

#include <libpdbg.h>

/**
 * @brief Parse a list of path target strings
 *
 * @param[in]  arg An array of strings
 * @param[in]  arg_count The number of strings
 * @return true on success, false on error
 */
bool path_target_parse(const char **arg, int arg_count);

/**
 * @brief Check if there are any path targets
 *
 * @return true if number of path targets > 0, false otherwise
 */
bool path_target_present(void);

/**
 * @brief Add a target to the list
 *
 * @param[in]  target pdbg target
 * @return true on success, false on error
 */
bool path_target_add(struct pdbg_target *target);

/**
 * @brief Check if the specified target is selected
 *
 * @param[in]  target pdbg target
 * @return true if the target is selected, false otherwise
 */
bool path_target_selected(struct pdbg_target *target);

/**
 * @brief Print the list of path targets to stdout
 */
void path_target_dump(void);

/**
 * @brief Iterator for list of path targets
 *
 * @param[in]  prev The last pdbg target
 * @return the next pdbg target in the list, NULL otherwise
 *
 * If prev is NULL, then return the first pdbg target in the list.
 * If there are no more pdbg targets in the list, NULL is returned.
 */
struct pdbg_target *path_target_next(struct pdbg_target *prev);

/**
 * @brief Iterator for a list of path targets of specified class
 *
 * @param[in]  klass The class of the targets required
 * @param[in]  prev The last pdbg target
 * @return the next matching pdbg target in the list, NULL otherwise
 *
 * If prev is NULL, then return the first matching pdbg target in the list.
 * If there are no more matching pdbg targets, NULL is returned.
 */
struct pdbg_target *path_target_next_class(const char *klass,
					   struct pdbg_target *prev);

/**
 * @brief Macro for iterating through all path targets
 *
 * target is of type struct pdbg_target
 */
#define for_each_path_target(target)           \
	for (target = path_target_next(NULL);  \
	     target;                           \
	     target = path_target_next(target))

/**
 * @brief Macro for iterating through all path targets of specific class
 *
 * class is of type char *
 * target is of type struct pdbg_target
 */
#define for_each_path_target_class(class, target)           \
	for (target = path_target_next_class(class, NULL);  \
	     target;                                        \
	     target = path_target_next_class(class, target))

#endif
