/* Copyright 2016 IBM Corp.
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
#include <stdint.h>
#include <stdio.h>

#include "bmcfsi.h"
#include "operations.h"

struct scom_backend *backend = NULL;

int backend_init(int slave_id)
{
	backend = fsi_init(slave_id);
	if (!backend)
		return -1;

	return 0;
}

void backend_destroy(void)
{
	if (!backend || backend->destroy)
		backend->destroy(backend);
}

int getscom(uint64_t *value, uint32_t addr)
{
	if (!backend || !backend->getscom) {
		PR_ERROR("Backend does not support %s\n", __FUNCTION__);
		return -1;
	}

	return backend->getscom(backend, value, addr);
}

int putscom(uint64_t value, uint32_t addr)
{
	if (!backend || !backend->getscom) {
		PR_ERROR("Backend does not support %s\n", __FUNCTION__);
		return -1;
	}

	return backend->putscom(backend, value, addr);
}

int getcfam(uint32_t *value, uint32_t addr)
{
	if (!backend || !backend->getscom) {
		PR_ERROR("Backend does not support %s\n", __FUNCTION__);
		return -1;
	}

	return backend->getcfam(backend, value, addr);
}

int putcfam(uint32_t value, uint32_t addr)
{
	if (!backend || !backend->putscom) {
		PR_ERROR("Backend does not support %s\n", __FUNCTION__);
		return -1;
	}

	return backend->putcfam(backend, value, addr);
}
