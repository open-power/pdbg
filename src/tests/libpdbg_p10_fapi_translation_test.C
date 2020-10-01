#include <stdio.h>
#include <inttypes.h>

#define class klass
#include "libpdbg/libpdbg.h"
#include "libpdbg/hwunit.h"
#include "libpdbg/target.h"
#undef class

#include "p10_scominfo.H"

#define MAX_INDEX 30

int test_unit_translation(struct pdbg_target *target, p10ChipUnits_t cu, int index, uint64_t addr)
{
	uint64_t pdbg_addr, fapi_addr;

	target->index = index;

	/* TODO: Check standard chiplet translation */
	if (!target->translate)
		return 1;

	if (validateChipUnitNum(index, cu))
		return 1;

	pdbg_addr = target->translate(target, addr);
	fapi_addr = p10_scominfo_createChipUnitScomAddr(cu, 0x10, index, addr, 0);

	/* Ignore bad addresses. We should really test that we get an assert error
	 * from the translation code though. */
	if (fapi_addr == FAILED_TRANSLATION)
		return 1;

	if (pdbg_addr != fapi_addr)
		fprintf(stderr,
			"PDBG Address 0x%016" PRIx64 " does not match FAPI Address 0x%016" PRIx64
			" for address 0x%016" PRIx64 " on target %s@%d\n",
			pdbg_addr, fapi_addr, addr, pdbg_target_path(target), index);

	return pdbg_addr == fapi_addr;
}

static struct chip_unit {
	p10ChipUnits_t cu;
	const char *classname;
} chip_unit[] = {
	{ PU_C_CHIPUNIT, "core" },
	{ PU_EQ_CHIPUNIT, "eq" },
	{ PU_PEC_CHIPUNIT, "pec" },
	{ PU_PHB_CHIPUNIT, "phb" },
	{ PU_MI_CHIPUNIT, "mi" },
	{ PU_MCC_CHIPUNIT, "mcc" },
	{ PU_OMIC_CHIPUNIT, "omic" },
	{ PU_OMI_CHIPUNIT, "omi" },
	{ PU_PERV_CHIPUNIT, "chiplet" },
	{ PU_MC_CHIPUNIT, "mc" },
	{ PU_NMMU_CHIPUNIT, "nmmu" },
	{ PU_IOHS_CHIPUNIT, "iohs" },
	{ PU_PAU_CHIPUNIT, "pau" },
	{ PU_PAUC_CHIPUNIT, "pauc" },
	{ NONE, NULL },
};

int main(int argc, const char **argv)
{
	struct pdbg_target *target;
	p10ChipUnits_t cu = NONE;
	int i, count=0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <class>\n", argv[0]);
		return 1;
	}

	// pdbg_set_loglevel(PDBG_DEBUG);

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
