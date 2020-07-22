/* Copyright 2020 IBM Corp.
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

#include <stdbool.h>
#include <string.h>

#include "libpdbg.h"
#include "sprs.h"

static struct {
	const char *spr_name;
	int spr_id;
} spr_map[] = {
	{ "XER", SPR_XER },
	{ "DSCR_RU", SPR_DSCR_RU },
	{ "LR", SPR_LR },
	{ "CTR", SPR_CTR },
	{ "UAMR", SPR_UAMR },
	{ "DSCR", SPR_DSCR },
	{ "DSISR", SPR_DSISR },
	{ "DAR", SPR_DAR },
	{ "DEC", SPR_DEC },
	{ "SDR1", SPR_SDR1 },
	{ "SRR0", SPR_SRR0 },
	{ "SRR1", SPR_SRR1 },
	{ "CFAR", SPR_CFAR },
	{ "AMR", SPR_AMR },
	{ "PIDR", SPR_PIDR },
	{ "IAMR", SPR_IAMR },
	{ "TFHAR", SPR_TFHAR },
	{ "TFIAR", SPR_TFIAR },
	{ "TEXASR", SPR_TEXASR },
	{ "TEXASRU", SPR_TEXASRU },
	{ "CTRL_RU", SPR_CTRL_RU },
	{ "TIDR", SPR_TIDR },
	{ "CTRL", SPR_CTRL },
	{ "FSCR", SPR_FSCR },
	{ "UAMOR", SPR_UAMOR },
	{ "GSR", SPR_GSR },
	{ "PSPB", SPR_PSPB },
	{ "DPDES", SPR_DPDES },
	{ "DAWR0", SPR_DAWR0 },
	{ "RPR", SPR_RPR },
	{ "CIABR", SPR_CIABR },
	{ "DAWRX0", SPR_DAWRX0 },
	{ "HFSCR", SPR_HFSCR },
	{ "VRSAVE", SPR_VRSAVE },
	{ "SPRG3_RU", SPR_SPRG3_RU },
	{ "TB", SPR_TB },
	{ "TBU_RU", SPR_TBU_RU },
	{ "SPRG0", SPR_SPRG0 },
	{ "SPRG1", SPR_SPRG1 },
	{ "SPRG2", SPR_SPRG2 },
	{ "SPRG3", SPR_SPRG3 },
	{ "SPRC", SPR_SPRC },
	{ "SPRD", SPR_SPRD },
	{ "CIR", SPR_CIR },
	{ "TBL", SPR_TBL },
	{ "TBU", SPR_TBU },
	{ "TBU40", SPR_TBU40 },
	{ "PVR", SPR_PVR },
	{ "HSPRG0", SPR_HSPRG0 },
	{ "HSPRG1", SPR_HSPRG1 },
	{ "HDSISR", SPR_HDSISR },
	{ "HDAR", SPR_HDAR },
	{ "SPURR", SPR_SPURR },
	{ "PURR", SPR_PURR },
	{ "HDEC", SPR_HDEC },
	{ "HRMOR", SPR_HRMOR },
	{ "HSRR0", SPR_HSRR0 },
	{ "HSRR1", SPR_HSRR1 },
	{ "TFMR", SPR_TFMR },
	{ "LPCR", SPR_LPCR },
	{ "LPIDR", SPR_LPIDR },
	{ "HMER", SPR_HMER },
	{ "HMEER", SPR_HMEER },
	{ "PCR", SPR_PCR },
	{ "HEIR", SPR_HEIR },
	{ "AMOR", SPR_AMOR },
	{ "TIR", SPR_TIR },
	{ "PTCR", SPR_PTCR },
	{ "USPRG0", SPR_USPRG0 },
	{ "USPRG1", SPR_USPRG1 },
	{ "UDAR", SPR_UDAR },
	{ "SEIDR", SPR_SEIDR },
	{ "URMOR", SPR_URMOR },
	{ "USRR0", SPR_USRR0 },
	{ "USRR1", SPR_USRR1 },
	{ "UEIR", SPR_UEIR },
	{ "ACMCR", SPR_ACMCR },
	{ "SMFCTRL", SPR_SMFCTRL },
	{ "SIER_RU", SPR_SIER_RU },
	{ "MMCR2_RU", SPR_MMCR2_RU },
	{ "MMCRA_RU", SPR_MMCRA_RU },
	{ "PMC1_RU", SPR_PMC1_RU },
	{ "PMC2_RU", SPR_PMC2_RU },
	{ "PMC3_RU", SPR_PMC3_RU },
	{ "PMC4_RU", SPR_PMC4_RU },
	{ "PMC5_RU", SPR_PMC5_RU },
	{ "PMC6_RU", SPR_PMC6_RU },
	{ "MMCR0_RU", SPR_MMCR0_RU },
	{ "SIAR_RU", SPR_SIAR_RU },
	{ "SDAR_RU", SPR_SDAR_RU },
	{ "MMCR1_RU", SPR_MMCR1_RU },
	{ "SIER", SPR_SIER },
	{ "MMCR2", SPR_MMCR2 },
	{ "MMCRA", SPR_MMCRA },
	{ "PMC1", SPR_PMC1 },
	{ "PMC2", SPR_PMC2 },
	{ "PMC3", SPR_PMC3 },
	{ "PMC4", SPR_PMC4 },
	{ "PMC5", SPR_PMC5 },
	{ "PMC6", SPR_PMC6 },
	{ "MMCR0", SPR_MMCR0 },
	{ "SIAR", SPR_SIAR },
	{ "SDAR", SPR_SDAR },
	{ "MMCR1", SPR_MMCR1 },
	{ "IMC", SPR_IMC },
	{ "BESCRS", SPR_BESCRS },
	{ "BESCRSU", SPR_BESCRSU },
	{ "BESCRR", SPR_BESCRR },
	{ "BESCRRU", SPR_BESCRRU },
	{ "EBBHR", SPR_EBBHR },
	{ "EBBRR", SPR_EBBRR },
	{ "BESCR", SPR_BESCR },
	{ "LMRR", SPR_LMRR },
	{ "LMSER", SPR_LMSER },
	{ "TAR", SPR_TAR },
	{ "ASDR", SPR_ASDR },
	{ "PSSCR_SU", SPR_PSSCR_SU },
	{ "IC", SPR_IC },
	{ "VTB", SPR_VTB },
	{ "LDBAR", SPR_LDBAR },
	{ "MMCRC", SPR_MMCRC },
	{ "PMSR", SPR_PMSR },
	{ "PMMAR", SPR_PMMAR },
	{ "PSSCR", SPR_PSSCR },
	{ "L2QOSR", SPR_L2QOSR },
	{ "WORC", SPR_WORC },
	{ "TRIG0", SPR_TRIG0 },
	{ "TRIG1", SPR_TRIG1 },
	{ "TRIG2", SPR_TRIG2 },
	{ "PMCR", SPR_PMCR },
	{ "RWMR", SPR_RWMR },
	{ "WORT", SPR_WORT },
	{ "PPR", SPR_PPR },
	{ "PPR32", SPR_PPR32 },
	{ "TSCR", SPR_TSCR },
	{ "TTR", SPR_TTR },
	{ "TRACE", SPR_TRACE },
	{ "HID", SPR_HID },
	{ "PIR", SPR_PIR },
	{ "NIA", SPR_NIA },
	{ "MSR", SPR_MSR },
	{ "CR", SPR_CR },
	{ "FPSCR", SPR_FPSCR },
	{ "VSCR", SPR_VSCR },
	{ "SLBE", SPR_SLBE },
	{ "SLBV", SPR_SLBV },
	{ NULL, -1 },
};

int pdbg_spr_by_name(const char *name)
{
	int i;

	for (i = 0; spr_map[i].spr_name != NULL; i++) {
		if (!strcasecmp(spr_map[i].spr_name, name)) {
			return spr_map[i].spr_id;
		}
	}

	return -1;
}

const char *pdbg_spr_by_id(int spr)
{
	int i;

	for (i = 0; spr_map[i].spr_name != NULL; i++) {
		if (spr_map[i].spr_id == spr) {
			return spr_map[i].spr_name;
		}
	}

	return NULL;
}
