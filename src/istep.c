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
#include <stdint.h>

#include <libpdbg.h>

#include "optcmd.h"
#include "path.h"

static int istep(uint32_t major, uint32_t minor)
{
	struct pdbg_target *target;
	int count = 0;

	if (major < 2 || major > 5) {
		fprintf(stderr, "Istep major should be 2 to 5\n");
		return 0;
	}
	if (major == 2 && minor == 1) {
		fprintf(stderr, "Istep 2 minor should be > 1\n");
		return 0;
	}

	for_each_path_target_class("sbefifo", target) {
		int rc;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		rc = sbe_istep(target, major, minor);
		if (!rc)
			count++;
	}

	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(istep, istep, (DATA32, DATA32));
