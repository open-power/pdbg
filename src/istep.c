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

struct istep_data {
	int major;
	int minor_first;
	int minor_last;
} istep_data[] = {
	{ 2, 2, 17 },
	{ 3, 1, 22 },
	{ 4, 1, 34 },
	{ 5, 1,  2 },
	{ 0, 0, 0  },
};

static int istep(uint32_t major, uint32_t minor)
{
	struct pdbg_target *target;
	int count = 0, i;
	int first = minor, last = minor;

	if (major < 2 || major > 5) {
		fprintf(stderr, "Istep major should be 2 to 5\n");
		return 0;
	}

	for (i=0; istep_data[i].major != 0; i++) {
		if (istep_data[i].major != major)
			continue;

		if (minor == 0) {
			first = istep_data[i].minor_first;
			last = istep_data[i].minor_last;
			break;
		}

		if (minor < istep_data[i].minor_first ||
		    minor > istep_data[i].minor_last) {
			fprintf(stderr, "Istep %d minor should be %d to %d\n",
				major,
				istep_data[i].minor_first,
				istep_data[i].minor_last);
			return 0;
		}
	}

	for_each_path_target_class("pib", target) {
		int rc;

		if (pdbg_target_status(target) != PDBG_TARGET_ENABLED)
			continue;

		for (i = first; i <= last ; i++) {
			printf("Running istep %d.%d\n", major, i);
			rc = sbe_istep(target, major, i);
			if (rc)
				goto fail;
		}

		count++;
	}

fail:
	return count;
}
OPTCMD_DEFINE_CMD_WITH_ARGS(istep, istep, (DATA32, DATA32));
