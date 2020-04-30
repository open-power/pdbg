/* copyright 2016 ibm corp.
 *
 * licensed under the apache license, version 2.0 (the "license");
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 * 	http://www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or
 * implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */
#include <stdio.h>
#include <inttypes.h>

#include "libpdbg.h"
#include "operations.h"
#include "hwunit.h"

static int fake_fsi_read(struct fsi *fsi, uint32_t addr, uint32_t *value)
{
	*value = 0xfeed0cfa;
	PR_DEBUG("fake_fsi_read(0x%04" PRIx32 ", 0x%04" PRIx32 ")\n", addr, *value);
	return 0;
}

static int fake_fsi_write(struct fsi *fsi, uint32_t addr, uint32_t value)
{
	PR_DEBUG("fake_fsi_write(0x%04" PRIx32 ", 0x%04" PRIx32 ")\n", addr, value);
	return 0;
}

static struct fsi fake_fsi = {
	.target = {
		.name =	"Fake FSI",
		.compatible = "ibm,fake-fsi",
		.class = "fsi",
	},
	.read = fake_fsi_read,
	.write = fake_fsi_write,
};
DECLARE_HW_UNIT(fake_fsi);

static int fake_pib_read(struct pib *pib, uint64_t addr, uint64_t *value)
{
	*value = 0xdeadbeef;
	PR_DEBUG("fake_pib_read(0x%08" PRIx64 ", 0x%08" PRIx64 ")\n", addr, *value);
	return 0;
}

static int fake_pib_write(struct pib *pib, uint64_t addr, uint64_t value)
{
	PR_DEBUG("fake_pib_write(0x%08" PRIx64 ", 0x%08" PRIx64 ")\n", addr, value);
	return 0;
}

static struct pib fake_pib = {
	.target = {
		.name =	"Fake PIB",
		.compatible = "ibm,fake-pib",
		.class = "pib",
	},
	.read = fake_pib_read,
	.write = fake_pib_write,
	.fd = -1,
};
DECLARE_HW_UNIT(fake_pib);

static struct core fake_core = {
	.target = {
		.name =	"Fake Core",
		.compatible = "ibm,fake-core",
		.class = "core",
	},
};
DECLARE_HW_UNIT(fake_core);

static struct thread fake_thread = {
	.target = {
		.name =	"Fake Thread",
		.compatible = "ibm,fake-thread",
		.class = "thread",
	},
};
DECLARE_HW_UNIT(fake_thread);

__attribute__((constructor))
static void register_fake(void)
{
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &fake_fsi_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &fake_pib_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &fake_core_hw_unit);
	pdbg_hwunit_register(PDBG_DEFAULT_BACKEND, &fake_thread_hw_unit);
}
