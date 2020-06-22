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
#ifndef __SPRS_H
#define __SPRS_H

/* From ekb/chips/p9/procedures/hwp/perv/p9_spr_name_map.H */
#define SPR_XER       1
#define SPR_DSCR_RU   3
#define SPR_LR        8
#define SPR_CTR       9
#define SPR_UAMR      13
#define SPR_DSCR      17
#define SPR_DSISR     18
#define SPR_DAR       19
#define SPR_DEC       22
#define SPR_SDR1      25
#define SPR_SRR0      26
#define SPR_SRR1      27
#define SPR_CFAR      28
#define SPR_AMR       29
#define SPR_PIDR      48
#define SPR_IAMR      61
#define SPR_TFHAR     128
#define SPR_TFIAR     129
#define SPR_TEXASR    130
#define SPR_TEXASRU   131
#define SPR_CTRL_RU   136
#define SPR_TIDR      144
#define SPR_CTRL      152
#define SPR_FSCR      153
#define SPR_UAMOR     157
#define SPR_GSR       158
#define SPR_PSPB      159
#define SPR_DPDES     176
#define SPR_DAWR0     180
#define SPR_RPR       186
#define SPR_CIABR     187
#define SPR_DAWRX0    188
#define SPR_HFSCR     190
#define SPR_VRSAVE    256
#define SPR_SPRG3_RU  259
#define SPR_TB        268
#define SPR_TBU_RU    269
#define SPR_SPRG0     272
#define SPR_SPRG1     273
#define SPR_SPRG2     274
#define SPR_SPRG3     275
#define SPR_SPRC      276
#define SPR_SPRD      277
#define SPR_CIR       283
#define SPR_TBL       284
#define SPR_TBU       285
#define SPR_TBU40     286
#define SPR_PVR       287
#define SPR_HSPRG0    304
#define SPR_HSPRG1    305
#define SPR_HDSISR    306
#define SPR_HDAR      307
#define SPR_SPURR     308
#define SPR_PURR      309
#define SPR_HDEC      310
#define SPR_HRMOR     313
#define SPR_HSRR0     314
#define SPR_HSRR1     315
#define SPR_TFMR      317
#define SPR_LPCR      318
#define SPR_LPIDR     319
#define SPR_HMER      336
#define SPR_HMEER     337
#define SPR_PCR       338
#define SPR_HEIR      339
#define SPR_AMOR      349
#define SPR_TIR       446
#define SPR_PTCR      464
#define SPR_USPRG0    496
#define SPR_USPRG1    497
#define SPR_UDAR      499
#define SPR_SEIDR     504
#define SPR_URMOR     505
#define SPR_USRR0     506
#define SPR_USRR1     507
#define SPR_UEIR      509
#define SPR_ACMCR     510
#define SPR_SMFCTRL   511
#define SPR_SIER_RU   768
#define SPR_MMCR2_RU  769
#define SPR_MMCRA_RU  770
#define SPR_PMC1_RU   771
#define SPR_PMC2_RU   772
#define SPR_PMC3_RU   773
#define SPR_PMC4_RU   774
#define SPR_PMC5_RU   775
#define SPR_PMC6_RU   776
#define SPR_MMCR0_RU  779
#define SPR_SIAR_RU   780
#define SPR_SDAR_RU   781
#define SPR_MMCR1_RU  782
#define SPR_SIER      784
#define SPR_MMCR2     785
#define SPR_MMCRA     786
#define SPR_PMC1      787
#define SPR_PMC2      788
#define SPR_PMC3      789
#define SPR_PMC4      790
#define SPR_PMC5      791
#define SPR_PMC6      792
#define SPR_MMCR0     795
#define SPR_SIAR      796
#define SPR_SDAR      797
#define SPR_MMCR1     798
#define SPR_IMC       799
#define SPR_BESCRS    800
#define SPR_BESCRSU   801
#define SPR_BESCRR    802
#define SPR_BESCRRU   803
#define SPR_EBBHR     804
#define SPR_EBBRR     805
#define SPR_BESCR     806
#define SPR_LMRR      813
#define SPR_LMSER     814
#define SPR_TAR       815
#define SPR_ASDR      816
#define SPR_PSSCR_SU  823
#define SPR_IC        848
#define SPR_VTB       849
#define SPR_LDBAR     850
#define SPR_MMCRC     851
#define SPR_PMSR      853
#define SPR_PMMAR     854
#define SPR_PSSCR     855
#define SPR_L2QOSR    861
#define SPR_WORC      863
#define SPR_TRIG0     880
#define SPR_TRIG1     881
#define SPR_TRIG2     882
#define SPR_PMCR      884
#define SPR_RWMR      885
#define SPR_WORT      895
#define SPR_PPR       896
#define SPR_PPR32     898
#define SPR_TSCR      921
#define SPR_TTR       922
#define SPR_TRACE     1006
#define SPR_HID       1008
#define SPR_PIR       1023
#define SPR_NIA       2000
#define SPR_MSR       2001
#define SPR_CR        2002
#define SPR_FPSCR     2003
#define SPR_VSCR      2004
#define SPR_SLBE      2005
#define SPR_SLBV      2006

#endif /* __SPRS_H */
