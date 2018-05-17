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
 */

#ifndef __PROGRESS_H
#define __PROGRESS_H

#include <inttypes.h>

void progress_init(uint64_t count);
void progress_tick(uint64_t cur);
void progress_end(void);

#endif /* __PROGRESS_H */
