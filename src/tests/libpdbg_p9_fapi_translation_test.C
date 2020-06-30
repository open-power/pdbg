#include <stdio.h>
#include <inttypes.h>

#define class klass
#include "libpdbg/libpdbg.h"
#include "libpdbg/hwunit.h"
#include "libpdbg/target.h"
#undef class

#include "p9_scominfo.H"

#define MAX_INDEX 30

int test_unit_translation(struct pdbg_target *target, p9ChipUnits_t cu, int index, uint64_t addr)
{
	uint64_t pdbg_addr, fapi_addr;

	target->index = index;

	/* TODO: Check standard chiplet translation */
	if (!target->translate)
		return 1;

	pdbg_addr = target->translate(target, addr);
	fapi_addr = p9_scominfo_createChipUnitScomAddr(cu, index, addr, 0);

	if (pdbg_addr != fapi_addr)
		fprintf(stderr,
			"PDBG Address 0x%016" PRIx64 " does not match FAPI Address 0x%016" PRIx64
		        " for address 0x%016" PRIx64 " on target %s@%d\n",
		        pdbg_addr, fapi_addr, addr, pdbg_target_path(target), index);

	return pdbg_addr == fapi_addr;
}

static struct chip_unit {
	p9ChipUnits_t cu;
	const char *classname;
} chip_unit[] = {
	{ P9C_CHIP, "" },
	{ P9N_CHIP, "" },
	{ P9A_CHIP, "" },
	{ PU_C_CHIPUNIT, "core" },
	{ PU_EQ_CHIPUNIT, "eq" },
	{ PU_EX_CHIPUNIT, "ex" },
	{ PU_XBUS_CHIPUNIT, "xbus" },
	{ PU_OBUS_CHIPUNIT, "obus" },
	{ PU_NV_CHIPUNIT, "nv" },
	{ PU_PEC_CHIPUNIT, "pec" },
	{ PU_PHB_CHIPUNIT, "phb" },
	{ PU_MI_CHIPUNIT, "mi" },
	{ PU_DMI_CHIPUNIT, "dmi" },
	{ PU_MCC_CHIPUNIT, "mcc" },
	{ PU_OMIC_CHIPUNIT, "omic" },
	{ PU_OMI_CHIPUNIT, "omi" },
	{ PU_MCS_CHIPUNIT, "mcs" },
	{ PU_MCA_CHIPUNIT, "mca" },
	{ PU_MCBIST_CHIPUNIT, "mcbist" },
	{ PU_PERV_CHIPUNIT, "chiplet" },
	{ PU_PPE_CHIPUNIT, "ppe" },
	{ PU_SBE_CHIPUNIT, "sbe" },
	{ PU_CAPP_CHIPUNIT, "capp" },
	{ PU_MC_CHIPUNIT, "mc" },
	{ NONE, NULL },
};

int main(int argc, const char **argv)
{
	struct pdbg_target *target;
	p9ChipUnits_t cu = NONE;
	int i, count=0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <class>\n", argv[0]);
		return 1;
	}

	// pdbg_set_logfunc(NULL);

	pdbg_targets_init(NULL);

	for (i=0; chip_unit[i].classname; i++) {
		if (!strcmp(argv[1], chip_unit[i].classname)) {
			cu = chip_unit[i].cu;
		}
	}

	if (cu == NONE) {
		fprintf(stderr, "Unknown class '%s'\n", argv[1]);
		return 1;
	}

	pdbg_for_each_class_target(argv[1], target) {
		uint64_t addr;
		int index = pdbg_target_index(target);

		/*  We only need to test targets on proc0, translation won't change for
		 *  other procs */
		if (pdbg_target_index(pdbg_target_parent("proc", target)))
			continue;

		printf("Testing %s  %d\n", pdbg_target_path(target), index);
		/* Test every sat offset */
		for (addr = 0; addr < 0xffffffff; addr += 0x40)
			assert(test_unit_translation(target, cu, index, addr));

		count++;
	}

	if (count == 0) {
		printf("Test skipped for class '%s'\n", argv[1]);
	}

	return 0;
}
