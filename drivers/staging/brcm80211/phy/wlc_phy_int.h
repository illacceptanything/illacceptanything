/*
 * Copyright (c) 2010 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _wlc_phy_int_h_
#define _wlc_phy_int_h_

#include <linux/kernel.h>
#include <bcmdefs.h>
#include <bcmutils.h>

#include <bcmsrom_fmt.h>
#include <wlc_phy_hal.h>

#define PHYHAL_ERROR	0x0001
#define PHYHAL_TRACE	0x0002
#define PHYHAL_INFORM	0x0004

extern u32 phyhal_msg_level;

#define PHY_INFORM_ON()		(phyhal_msg_level & PHYHAL_INFORM)
#define PHY_THERMAL_ON()	(phyhal_msg_level & PHYHAL_THERMAL)
#define PHY_CAL_ON()		(phyhal_msg_level & PHYHAL_CAL)

#ifdef BOARD_TYPE
#define BOARDTYPE(_type) BOARD_TYPE
#else
#define BOARDTYPE(_type) _type
#endif

#define LCNXN_BASEREV		16

struct wlc_hw_info;
typedef struct phy_info phy_info_t;
typedef void (*initfn_t) (phy_info_t *);
typedef void (*chansetfn_t) (phy_info_t *, chanspec_t);
typedef int (*longtrnfn_t) (phy_info_t *, int);
typedef void (*txiqccgetfn_t) (phy_info_t *, u16 *, u16 *);
typedef void (*txiqccsetfn_t) (phy_info_t *, u16, u16);
typedef u16(*txloccgetfn_t) (phy_info_t *);
typedef void (*radioloftgetfn_t) (phy_info_t *, u8 *, u8 *, u8 *,
				  u8 *);
typedef s32(*rxsigpwrfn_t) (phy_info_t *, s32);
typedef void (*detachfn_t) (phy_info_t *);

#undef ISNPHY
#undef ISLCNPHY
#define ISNPHY(pi)	PHYTYPE_IS((pi)->pubpi.phy_type, PHY_TYPE_N)
#define ISLCNPHY(pi)  	PHYTYPE_IS((pi)->pubpi.phy_type, PHY_TYPE_LCN)

#define ISPHY_11N_CAP(pi)	(ISNPHY(pi) || ISLCNPHY(pi))

#define IS20MHZ(pi)	((pi)->bw == WL_CHANSPEC_BW_20)
#define IS40MHZ(pi)	((pi)->bw == WL_CHANSPEC_BW_40)

#define PHY_GET_RFATTN(rfgain)	((rfgain) & 0x0f)
#define PHY_GET_PADMIX(rfgain)	(((rfgain) & 0x10) >> 4)
#define PHY_GET_RFGAINID(rfattn, padmix, width)	((rfattn) + ((padmix)*(width)))
#define PHY_SAT(x, n)		((x) > ((1<<((n)-1))-1) ? ((1<<((n)-1))-1) : \
				((x) < -(1<<((n)-1)) ? -(1<<((n)-1)) : (x)))
#define PHY_SHIFT_ROUND(x, n)	((x) >= 0 ? ((x)+(1<<((n)-1)))>>(n) : (x)>>(n))
#define PHY_HW_ROUND(x, s)		((x >> s) + ((x >> (s-1)) & (s != 0)))

#define CH_5G_GROUP	3
#define A_LOW_CHANS	0
#define A_MID_CHANS	1
#define A_HIGH_CHANS	2
#define CH_2G_GROUP	1
#define G_ALL_CHANS	0

#define FIRST_REF5_CHANNUM	149
#define LAST_REF5_CHANNUM	165
#define	FIRST_5G_CHAN		14
#define	LAST_5G_CHAN		50
#define	FIRST_MID_5G_CHAN	14
#define	LAST_MID_5G_CHAN	35
#define	FIRST_HIGH_5G_CHAN	36
#define	LAST_HIGH_5G_CHAN	41
#define	FIRST_LOW_5G_CHAN	42
#define	LAST_LOW_5G_CHAN	50

#define BASE_LOW_5G_CHAN	4900
#define BASE_MID_5G_CHAN	5100
#define BASE_HIGH_5G_CHAN	5500

#define CHAN5G_FREQ(chan)  (5000 + chan*5)
#define CHAN2G_FREQ(chan)  (2407 + chan*5)

#define TXP_FIRST_CCK		0
#define TXP_LAST_CCK		3
#define TXP_FIRST_OFDM		4
#define TXP_LAST_OFDM		11
#define TXP_FIRST_OFDM_20_CDD	12
#define TXP_LAST_OFDM_20_CDD	19
#define TXP_FIRST_MCS_20_SISO	20
#define TXP_LAST_MCS_20_SISO	27
#define TXP_FIRST_MCS_20_CDD	28
#define TXP_LAST_MCS_20_CDD	35
#define TXP_FIRST_MCS_20_STBC	36
#define TXP_LAST_MCS_20_STBC	43
#define TXP_FIRST_MCS_20_SDM	44
#define TXP_LAST_MCS_20_SDM	51
#define TXP_FIRST_OFDM_40_SISO	52
#define TXP_LAST_OFDM_40_SISO	59
#define TXP_FIRST_OFDM_40_CDD	60
#define TXP_LAST_OFDM_40_CDD	67
#define TXP_FIRST_MCS_40_SISO	68
#define TXP_LAST_MCS_40_SISO	75
#define TXP_FIRST_MCS_40_CDD	76
#define TXP_LAST_MCS_40_CDD	83
#define TXP_FIRST_MCS_40_STBC	84
#define TXP_LAST_MCS_40_STBC	91
#define TXP_FIRST_MCS_40_SDM	92
#define TXP_LAST_MCS_40_SDM	99
#define TXP_MCS_32	        100
#define TXP_NUM_RATES		101
#define ADJ_PWR_TBL_LEN		84

#define TXP_FIRST_SISO_MCS_20	20
#define TXP_LAST_SISO_MCS_20	27

#define PHY_CORE_NUM_1	1
#define PHY_CORE_NUM_2	2
#define PHY_CORE_NUM_3	3
#define PHY_CORE_NUM_4	4
#define PHY_CORE_MAX	PHY_CORE_NUM_4
#define PHY_CORE_0	0
#define PHY_CORE_1	1
#define PHY_CORE_2	2
#define PHY_CORE_3	3

#define MA_WINDOW_SZ		8

#define PHY_NOISE_SAMPLE_MON		1
#define PHY_NOISE_SAMPLE_EXTERNAL	2
#define PHY_NOISE_WINDOW_SZ	16
#define PHY_NOISE_GLITCH_INIT_MA 10
#define PHY_NOISE_GLITCH_INIT_MA_BADPlCP 10
#define PHY_NOISE_STATE_MON		0x1
#define PHY_NOISE_STATE_EXTERNAL	0x2
#define PHY_NOISE_SAMPLE_LOG_NUM_NPHY	10
#define PHY_NOISE_SAMPLE_LOG_NUM_UCODE	9

#define PHY_NOISE_OFFSETFACT_4322  (-103)
#define PHY_NOISE_MA_WINDOW_SZ	2

#define	PHY_RSSI_TABLE_SIZE	64
#define RSSI_ANT_MERGE_MAX	0
#define RSSI_ANT_MERGE_MIN	1
#define RSSI_ANT_MERGE_AVG	2

#define	PHY_TSSI_TABLE_SIZE	64
#define	APHY_TSSI_TABLE_SIZE	256
#define	TX_GAIN_TABLE_LENGTH	64
#define	DEFAULT_11A_TXP_IDX	24
#define NUM_TSSI_FRAMES        4
#define	NULL_TSSI		0x7f
#define	NULL_TSSI_W		0x7f7f

#define PHY_PAPD_EPS_TBL_SIZE_LCNPHY 64

#define LCNPHY_PERICAL_TEMPBASED_TXPWRCTRL 9

#define PHY_TXPWR_MIN		10
#define PHY_TXPWR_MIN_NPHY	8
#define RADIOPWR_OVERRIDE_DEF	(-1)

#define PWRTBL_NUM_COEFF	3

#define SPURAVOID_DISABLE	0
#define SPURAVOID_AUTO		1
#define SPURAVOID_FORCEON	2
#define SPURAVOID_FORCEON2	3

#define PHY_SW_TIMER_FAST		15
#define PHY_SW_TIMER_SLOW		60
#define PHY_SW_TIMER_GLACIAL	120

#define PHY_PERICAL_AUTO	0
#define PHY_PERICAL_FULL	1
#define PHY_PERICAL_PARTIAL	2

#define PHY_PERICAL_NODELAY	0
#define PHY_PERICAL_INIT_DELAY	5
#define PHY_PERICAL_ASSOC_DELAY	5
#define PHY_PERICAL_WDOG_DELAY	5

#define MPHASE_TXCAL_NUMCMDS	2
#define PHY_PERICAL_MPHASE_PENDING(pi)	(pi->mphase_cal_phase_id > MPHASE_CAL_STATE_IDLE)

enum {
	MPHASE_CAL_STATE_IDLE = 0,
	MPHASE_CAL_STATE_INIT = 1,
	MPHASE_CAL_STATE_TXPHASE0,
	MPHASE_CAL_STATE_TXPHASE1,
	MPHASE_CAL_STATE_TXPHASE2,
	MPHASE_CAL_STATE_TXPHASE3,
	MPHASE_CAL_STATE_TXPHASE4,
	MPHASE_CAL_STATE_TXPHASE5,
	MPHASE_CAL_STATE_PAPDCAL,
	MPHASE_CAL_STATE_RXCAL,
	MPHASE_CAL_STATE_RSSICAL,
	MPHASE_CAL_STATE_IDLETSSI
};

typedef enum {
	CAL_FULL,
	CAL_RECAL,
	CAL_CURRECAL,
	CAL_DIGCAL,
	CAL_GCTRL,
	CAL_SOFT,
	CAL_DIGLO
} phy_cal_mode_t;

#define RDR_NTIERS  1
#define RDR_TIER_SIZE 64
#define RDR_LIST_SIZE (512/3)
#define RDR_EPOCH_SIZE 40
#define RDR_NANTENNAS 2
#define RDR_NTIER_SIZE  RDR_LIST_SIZE
#define RDR_LP_BUFFER_SIZE 64
#define LP_LEN_HIS_SIZE 10

#define STATIC_NUM_RF 32
#define STATIC_NUM_BB 9

#define BB_MULT_MASK		0x0000ffff
#define BB_MULT_VALID_MASK	0x80000000

#define CORDIC_AG	39797
#define	CORDIC_NI	18
#define	FIXED(X)	((s32)((X) << 16))
#define	FLOAT(X)	(((X) >= 0) ? ((((X) >> 15) + 1) >> 1) : -((((-(X)) >> 15) + 1) >> 1))

#define PHY_CHAIN_TX_DISABLE_TEMP	115
#define PHY_HYSTERESIS_DELTATEMP	5

#define PHY_BITSCNT(x)	bcm_bitcount((u8 *)&(x), sizeof(u8))

#define MOD_PHY_REG(pi, phy_type, reg_name, field, value) \
	mod_phy_reg(pi, phy_type##_##reg_name, phy_type##_##reg_name##_##field##_MASK, \
	(value) << phy_type##_##reg_name##_##field##_##SHIFT);
#define READ_PHY_REG(pi, phy_type, reg_name, field) \
	((read_phy_reg(pi, phy_type##_##reg_name) & phy_type##_##reg_name##_##field##_##MASK)\
	>> phy_type##_##reg_name##_##field##_##SHIFT)

#define	VALID_PHYTYPE(phytype)	(((uint)phytype == PHY_TYPE_N) || \
				((uint)phytype == PHY_TYPE_LCN))

#define VALID_N_RADIO(radioid)	((radioid == BCM2055_ID) || (radioid == BCM2056_ID) || \
				(radioid == BCM2057_ID))
#define VALID_LCN_RADIO(radioid)	(radioid == BCM2064_ID)

#define	VALID_RADIO(pi, radioid)	(\
	(ISNPHY(pi) ? VALID_N_RADIO(radioid) : false) || \
	(ISLCNPHY(pi) ? VALID_LCN_RADIO(radioid) : false))

#define SCAN_INPROG_PHY(pi)	(mboolisset(pi->measure_hold, PHY_HOLD_FOR_SCAN))
#define RM_INPROG_PHY(pi)	(mboolisset(pi->measure_hold, PHY_HOLD_FOR_RM))
#define PLT_INPROG_PHY(pi)	(mboolisset(pi->measure_hold, PHY_HOLD_FOR_PLT))
#define ASSOC_INPROG_PHY(pi)	(mboolisset(pi->measure_hold, PHY_HOLD_FOR_ASSOC))
#define SCAN_RM_IN_PROGRESS(pi) (mboolisset(pi->measure_hold, PHY_HOLD_FOR_SCAN | PHY_HOLD_FOR_RM))
#define PHY_MUTED(pi)		(mboolisset(pi->measure_hold, PHY_HOLD_FOR_MUTE))
#define PUB_NOT_ASSOC(pi)	(mboolisset(pi->measure_hold, PHY_HOLD_FOR_NOT_ASSOC))

#if defined(EXT_CBALL)
#define NORADIO_ENAB(pub) ((pub).radioid == NORADIO_ID)
#else
#define NORADIO_ENAB(pub) 0
#endif

#define PHY_LTRN_LIST_LEN	64
extern u16 ltrn_list[PHY_LTRN_LIST_LEN];

typedef struct _phy_table_info {
	uint table;
	int q;
	uint max;
} phy_table_info_t;

typedef struct phytbl_info {
	const void *tbl_ptr;
	u32 tbl_len;
	u32 tbl_id;
	u32 tbl_offset;
	u32 tbl_width;
} phytbl_info_t;

typedef struct {
	u8 curr_home_channel;
	u16 crsminpwrthld_40_stored;
	u16 crsminpwrthld_20L_stored;
	u16 crsminpwrthld_20U_stored;
	u16 init_gain_code_core1_stored;
	u16 init_gain_code_core2_stored;
	u16 init_gain_codeb_core1_stored;
	u16 init_gain_codeb_core2_stored;
	u16 init_gain_table_stored[4];

	u16 clip1_hi_gain_code_core1_stored;
	u16 clip1_hi_gain_code_core2_stored;
	u16 clip1_hi_gain_codeb_core1_stored;
	u16 clip1_hi_gain_codeb_core2_stored;
	u16 nb_clip_thresh_core1_stored;
	u16 nb_clip_thresh_core2_stored;
	u16 init_ofdmlna2gainchange_stored[4];
	u16 init_ccklna2gainchange_stored[4];
	u16 clip1_lo_gain_code_core1_stored;
	u16 clip1_lo_gain_code_core2_stored;
	u16 clip1_lo_gain_codeb_core1_stored;
	u16 clip1_lo_gain_codeb_core2_stored;
	u16 w1_clip_thresh_core1_stored;
	u16 w1_clip_thresh_core2_stored;
	u16 radio_2056_core1_rssi_gain_stored;
	u16 radio_2056_core2_rssi_gain_stored;
	u16 energy_drop_timeout_len_stored;

	u16 ed_crs40_assertthld0_stored;
	u16 ed_crs40_assertthld1_stored;
	u16 ed_crs40_deassertthld0_stored;
	u16 ed_crs40_deassertthld1_stored;
	u16 ed_crs20L_assertthld0_stored;
	u16 ed_crs20L_assertthld1_stored;
	u16 ed_crs20L_deassertthld0_stored;
	u16 ed_crs20L_deassertthld1_stored;
	u16 ed_crs20U_assertthld0_stored;
	u16 ed_crs20U_assertthld1_stored;
	u16 ed_crs20U_deassertthld0_stored;
	u16 ed_crs20U_deassertthld1_stored;

	u16 badplcp_ma;
	u16 badplcp_ma_previous;
	u16 badplcp_ma_total;
	u16 badplcp_ma_list[MA_WINDOW_SZ];
	int badplcp_ma_index;
	s16 pre_badplcp_cnt;
	s16 bphy_pre_badplcp_cnt;

	u16 init_gain_core1;
	u16 init_gain_core2;
	u16 init_gainb_core1;
	u16 init_gainb_core2;
	u16 init_gain_rfseq[4];

	u16 crsminpwr0;
	u16 crsminpwrl0;
	u16 crsminpwru0;

	s16 crsminpwr_index;

	u16 radio_2057_core1_rssi_wb1a_gc_stored;
	u16 radio_2057_core2_rssi_wb1a_gc_stored;
	u16 radio_2057_core1_rssi_wb1g_gc_stored;
	u16 radio_2057_core2_rssi_wb1g_gc_stored;
	u16 radio_2057_core1_rssi_wb2_gc_stored;
	u16 radio_2057_core2_rssi_wb2_gc_stored;
	u16 radio_2057_core1_rssi_nb_gc_stored;
	u16 radio_2057_core2_rssi_nb_gc_stored;

} interference_info_t;

typedef struct {
	u16 rc_cal_ovr;
	u16 phycrsth1;
	u16 phycrsth2;
	u16 init_n1p1_gain;
	u16 p1_p2_gain;
	u16 n1_n2_gain;
	u16 n1_p1_gain;
	u16 div_search_gain;
	u16 div_p1_p2_gain;
	u16 div_search_gn_change;
	u16 table_7_2;
	u16 table_7_3;
	u16 cckshbits_gnref;
	u16 clip_thresh;
	u16 clip2_thresh;
	u16 clip3_thresh;
	u16 clip_p2_thresh;
	u16 clip_pwdn_thresh;
	u16 clip_n1p1_thresh;
	u16 clip_n1_pwdn_thresh;
	u16 bbconfig;
	u16 cthr_sthr_shdin;
	u16 energy;
	u16 clip_p1_p2_thresh;
	u16 threshold;
	u16 reg15;
	u16 reg16;
	u16 reg17;
	u16 div_srch_idx;
	u16 div_srch_p1_p2;
	u16 div_srch_gn_back;
	u16 ant_dwell;
	u16 ant_wr_settle;
} aci_save_gphy_t;

typedef struct _lo_complex_t {
	s8 i;
	s8 q;
} lo_complex_abgphy_info_t;

typedef struct _nphy_iq_comp {
	s16 a0;
	s16 b0;
	s16 a1;
	s16 b1;
} nphy_iq_comp_t;

typedef struct _nphy_txpwrindex {
	s8 index;
	s8 index_internal;
	s8 index_internal_save;
	u16 AfectrlOverride;
	u16 AfeCtrlDacGain;
	u16 rad_gain;
	u8 bbmult;
	u16 iqcomp_a;
	u16 iqcomp_b;
	u16 locomp;
} phy_txpwrindex_t;

typedef struct {

	u16 txcal_coeffs_2G[8];
	u16 txcal_radio_regs_2G[8];
	nphy_iq_comp_t rxcal_coeffs_2G;

	u16 txcal_coeffs_5G[8];
	u16 txcal_radio_regs_5G[8];
	nphy_iq_comp_t rxcal_coeffs_5G;
} txiqcal_cache_t;

typedef struct _nphy_pwrctrl {
	s8 max_pwr_2g;
	s8 idle_targ_2g;
	s16 pwrdet_2g_a1;
	s16 pwrdet_2g_b0;
	s16 pwrdet_2g_b1;
	s8 max_pwr_5gm;
	s8 idle_targ_5gm;
	s8 max_pwr_5gh;
	s8 max_pwr_5gl;
	s16 pwrdet_5gm_a1;
	s16 pwrdet_5gm_b0;
	s16 pwrdet_5gm_b1;
	s16 pwrdet_5gl_a1;
	s16 pwrdet_5gl_b0;
	s16 pwrdet_5gl_b1;
	s16 pwrdet_5gh_a1;
	s16 pwrdet_5gh_b0;
	s16 pwrdet_5gh_b1;
	s8 idle_targ_5gl;
	s8 idle_targ_5gh;
	s8 idle_tssi_2g;
	s8 idle_tssi_5g;
	s8 idle_tssi;
	s16 a1;
	s16 b0;
	s16 b1;
} phy_pwrctrl_t;

typedef struct _nphy_txgains {
	u16 txlpf[2];
	u16 txgm[2];
	u16 pga[2];
	u16 pad[2];
	u16 ipa[2];
} nphy_txgains_t;

#define PHY_NOISEVAR_BUFSIZE 10

typedef struct _nphy_noisevar_buf {
	int bufcount;
	int tone_id[PHY_NOISEVAR_BUFSIZE];
	u32 noise_vars[PHY_NOISEVAR_BUFSIZE];
	u32 min_noise_vars[PHY_NOISEVAR_BUFSIZE];
} phy_noisevar_buf_t;

typedef struct {
	u16 rssical_radio_regs_2G[2];
	u16 rssical_phyregs_2G[12];

	u16 rssical_radio_regs_5G[2];
	u16 rssical_phyregs_5G[12];
} rssical_cache_t;

typedef struct {

	u16 txiqlocal_a;
	u16 txiqlocal_b;
	u16 txiqlocal_didq;
	u8 txiqlocal_ei0;
	u8 txiqlocal_eq0;
	u8 txiqlocal_fi0;
	u8 txiqlocal_fq0;

	u16 txiqlocal_bestcoeffs[11];
	u16 txiqlocal_bestcoeffs_valid;

	u32 papd_eps_tbl[PHY_PAPD_EPS_TBL_SIZE_LCNPHY];
	u16 analog_gain_ref;
	u16 lut_begin;
	u16 lut_end;
	u16 lut_step;
	u16 rxcompdbm;
	u16 papdctrl;
	u16 sslpnCalibClkEnCtrl;

	u16 rxiqcal_coeff_a0;
	u16 rxiqcal_coeff_b0;
} lcnphy_cal_results_t;

struct shared_phy {
	struct phy_info *phy_head;
	uint unit;
	osl_t *osh;
	si_t *sih;
	void *physhim;
	uint corerev;
	u32 machwcap;
	bool up;
	bool clk;
	uint now;
	u16 vid;
	u16 did;
	uint chip;
	uint chiprev;
	uint chippkg;
	uint sromrev;
	uint boardtype;
	uint boardrev;
	uint boardvendor;
	u32 boardflags;
	u32 boardflags2;
	uint bustype;
	uint buscorerev;
	uint fast_timer;
	uint slow_timer;
	uint glacial_timer;
	u8 rx_antdiv;
	s8 phy_noise_window[MA_WINDOW_SZ];
	uint phy_noise_index;
	u8 hw_phytxchain;
	u8 hw_phyrxchain;
	u8 phytxchain;
	u8 phyrxchain;
	u8 rssi_mode;
	bool _rifs_phy;
};

struct phy_pub {
	uint phy_type;
	uint phy_rev;
	u8 phy_corenum;
	u16 radioid;
	u8 radiorev;
	u8 radiover;

	uint coreflags;
	uint ana_rev;
	bool abgphy_encore;
};

struct phy_info_nphy;
typedef struct phy_info_nphy phy_info_nphy_t;

struct phy_info_lcnphy;
typedef struct phy_info_lcnphy phy_info_lcnphy_t;

struct phy_func_ptr {
	initfn_t init;
	initfn_t calinit;
	chansetfn_t chanset;
	initfn_t txpwrrecalc;
	longtrnfn_t longtrn;
	txiqccgetfn_t txiqccget;
	txiqccsetfn_t txiqccset;
	txloccgetfn_t txloccget;
	radioloftgetfn_t radioloftget;
	initfn_t carrsuppr;
	rxsigpwrfn_t rxsigpwr;
	detachfn_t detach;
};
typedef struct phy_func_ptr phy_func_ptr_t;

struct phy_info {
	wlc_phy_t pubpi_ro;
	shared_phy_t *sh;
	phy_func_ptr_t pi_fptr;
	void *pi_ptr;

	union {
		phy_info_lcnphy_t *pi_lcnphy;
	} u;
	bool user_txpwr_at_rfport;

	d11regs_t *regs;
	struct phy_info *next;
	char *vars;
	wlc_phy_t pubpi;

	bool do_initcal;
	bool phytest_on;
	bool ofdm_rateset_war;
	bool bf_preempt_4306;
	chanspec_t radio_chanspec;
	u8 antsel_type;
	u16 bw;
	u8 txpwr_percent;
	bool phy_init_por;

	bool init_in_progress;
	bool initialized;
	bool sbtml_gm;
	uint refcnt;
	bool watchdog_override;
	u8 phynoise_state;
	uint phynoise_now;
	int phynoise_chan_watchdog;
	bool phynoise_polling;
	bool disable_percal;
	mbool measure_hold;

	s16 txpa_2g[PWRTBL_NUM_COEFF];
	s16 txpa_2g_low_temp[PWRTBL_NUM_COEFF];
	s16 txpa_2g_high_temp[PWRTBL_NUM_COEFF];
	s16 txpa_5g_low[PWRTBL_NUM_COEFF];
	s16 txpa_5g_mid[PWRTBL_NUM_COEFF];
	s16 txpa_5g_hi[PWRTBL_NUM_COEFF];

	u8 tx_srom_max_2g;
	u8 tx_srom_max_5g_low;
	u8 tx_srom_max_5g_mid;
	u8 tx_srom_max_5g_hi;
	u8 tx_srom_max_rate_2g[TXP_NUM_RATES];
	u8 tx_srom_max_rate_5g_low[TXP_NUM_RATES];
	u8 tx_srom_max_rate_5g_mid[TXP_NUM_RATES];
	u8 tx_srom_max_rate_5g_hi[TXP_NUM_RATES];
	u8 tx_user_target[TXP_NUM_RATES];
	s8 tx_power_offset[TXP_NUM_RATES];
	u8 tx_power_target[TXP_NUM_RATES];

	srom_fem_t srom_fem2g;
	srom_fem_t srom_fem5g;

	u8 tx_power_max;
	u8 tx_power_max_rate_ind;
	bool hwpwrctrl;
	u8 nphy_txpwrctrl;
	s8 nphy_txrx_chain;
	bool phy_5g_pwrgain;

	u16 phy_wreg;
	u16 phy_wreg_limit;

	s8 n_preamble_override;
	u8 antswitch;
	u8 aa2g, aa5g;

	s8 idle_tssi[CH_5G_GROUP];
	s8 target_idle_tssi;
	s8 txpwr_est_Pout;
	u8 tx_power_min;
	u8 txpwr_limit[TXP_NUM_RATES];
	u8 txpwr_env_limit[TXP_NUM_RATES];
	u8 adj_pwr_tbl_nphy[ADJ_PWR_TBL_LEN];

	bool channel_14_wide_filter;

	bool txpwroverride;
	bool txpwridx_override_aphy;
	s16 radiopwr_override;
	u16 hwpwr_txcur;
	u8 saved_txpwr_idx;

	bool edcrs_threshold_lock;

	u32 tr_R_gain_val;
	u32 tr_T_gain_val;

	s16 ofdm_analog_filt_bw_override;
	s16 cck_analog_filt_bw_override;
	s16 ofdm_rccal_override;
	s16 cck_rccal_override;
	u16 extlna_type;

	uint interference_mode_crs_time;
	u16 crsglitch_prev;
	bool interference_mode_crs;

	u32 phy_tx_tone_freq;
	uint phy_lastcal;
	bool phy_forcecal;
	bool phy_fixed_noise;
	u32 xtalfreq;
	u8 pdiv;
	s8 carrier_suppr_disable;

	bool phy_bphy_evm;
	bool phy_bphy_rfcs;
	s8 phy_scraminit;
	u8 phy_gpiosel;

	s16 phy_txcore_disable_temp;
	s16 phy_txcore_enable_temp;
	s8 phy_tempsense_offset;
	bool phy_txcore_heatedup;

	u16 radiopwr;
	u16 bb_atten;
	u16 txctl1;

	u16 mintxbias;
	u16 mintxmag;
	lo_complex_abgphy_info_t gphy_locomp_iq[STATIC_NUM_RF][STATIC_NUM_BB];
	s8 stats_11b_txpower[STATIC_NUM_RF][STATIC_NUM_BB];
	u16 gain_table[TX_GAIN_TABLE_LENGTH];
	bool loopback_gain;
	s16 max_lpback_gain_hdB;
	s16 trsw_rx_gain_hdB;
	u8 power_vec[8];

	u16 rc_cal;
	int nrssi_table_delta;
	int nrssi_slope_scale;
	int nrssi_slope_offset;
	int min_rssi;
	int max_rssi;

	s8 txpwridx;
	u8 min_txpower;

	u8 a_band_high_disable;

	u16 tx_vos;
	u16 global_tx_bb_dc_bias_loft;

	int rf_max;
	int bb_max;
	int rf_list_size;
	int bb_list_size;
	u16 *rf_attn_list;
	u16 *bb_attn_list;
	u16 padmix_mask;
	u16 padmix_reg;
	u16 *txmag_list;
	uint txmag_len;
	bool txmag_enable;

	s8 *a_tssi_to_dbm;
	s8 *m_tssi_to_dbm;
	s8 *l_tssi_to_dbm;
	s8 *h_tssi_to_dbm;
	u8 *hwtxpwr;

	u16 freqtrack_saved_regs[2];
	int cur_interference_mode;
	bool hwpwrctrl_capable;
	bool temppwrctrl_capable;

	uint phycal_nslope;
	uint phycal_noffset;
	uint phycal_mlo;
	uint phycal_txpower;

	u8 phy_aa2g;

	bool nphy_tableloaded;
	s8 nphy_rssisel;
	u32 nphy_bb_mult_save;
	u16 nphy_txiqlocal_bestc[11];
	bool nphy_txiqlocal_coeffsvalid;
	phy_txpwrindex_t nphy_txpwrindex[PHY_CORE_NUM_2];
	phy_pwrctrl_t nphy_pwrctrl_info[PHY_CORE_NUM_2];
	u16 cck2gpo;
	u32 ofdm2gpo;
	u32 ofdm5gpo;
	u32 ofdm5glpo;
	u32 ofdm5ghpo;
	u8 bw402gpo;
	u8 bw405gpo;
	u8 bw405glpo;
	u8 bw405ghpo;
	u8 cdd2gpo;
	u8 cdd5gpo;
	u8 cdd5glpo;
	u8 cdd5ghpo;
	u8 stbc2gpo;
	u8 stbc5gpo;
	u8 stbc5glpo;
	u8 stbc5ghpo;
	u8 bwdup2gpo;
	u8 bwdup5gpo;
	u8 bwdup5glpo;
	u8 bwdup5ghpo;
	u16 mcs2gpo[8];
	u16 mcs5gpo[8];
	u16 mcs5glpo[8];
	u16 mcs5ghpo[8];
	u32 nphy_rxcalparams;

	u8 phy_spuravoid;
	bool phy_isspuravoid;

	u8 phy_pabias;
	u8 nphy_papd_skip;
	u8 nphy_tssi_slope;

	s16 nphy_noise_win[PHY_CORE_MAX][PHY_NOISE_WINDOW_SZ];
	u8 nphy_noise_index;

	u8 nphy_txpid2g[PHY_CORE_NUM_2];
	u8 nphy_txpid5g[PHY_CORE_NUM_2];
	u8 nphy_txpid5gl[PHY_CORE_NUM_2];
	u8 nphy_txpid5gh[PHY_CORE_NUM_2];

	bool nphy_gain_boost;
	bool nphy_elna_gain_config;
	u16 old_bphy_test;
	u16 old_bphy_testcontrol;

	bool phyhang_avoid;

	bool rssical_nphy;
	u8 nphy_perical;
	uint nphy_perical_last;
	u8 cal_type_override;
	u8 mphase_cal_phase_id;
	u8 mphase_txcal_cmdidx;
	u8 mphase_txcal_numcmds;
	u16 mphase_txcal_bestcoeffs[11];
	chanspec_t nphy_txiqlocal_chanspec;
	chanspec_t nphy_iqcal_chanspec_2G;
	chanspec_t nphy_iqcal_chanspec_5G;
	chanspec_t nphy_rssical_chanspec_2G;
	chanspec_t nphy_rssical_chanspec_5G;
	struct wlapi_timer *phycal_timer;
	bool use_int_tx_iqlo_cal_nphy;
	bool internal_tx_iqlo_cal_tapoff_intpa_nphy;
	s16 nphy_lastcal_temp;

	txiqcal_cache_t calibration_cache;
	rssical_cache_t rssical_cache;

	u8 nphy_txpwr_idx[2];
	u8 nphy_papd_cal_type;
	uint nphy_papd_last_cal;
	u16 nphy_papd_tx_gain_at_last_cal[2];
	u8 nphy_papd_cal_gain_index[2];
	s16 nphy_papd_epsilon_offset[2];
	bool nphy_papd_recal_enable;
	u32 nphy_papd_recal_counter;
	bool nphy_force_papd_cal;
	bool nphy_papdcomp;
	bool ipa2g_on;
	bool ipa5g_on;

	u16 classifier_state;
	u16 clip_state[2];
	uint nphy_deaf_count;
	u8 rxiq_samps;
	u8 rxiq_antsel;

	u16 rfctrlIntc1_save;
	u16 rfctrlIntc2_save;
	bool first_cal_after_assoc;
	u16 tx_rx_cal_radio_saveregs[22];
	u16 tx_rx_cal_phy_saveregs[15];

	u8 nphy_cal_orig_pwr_idx[2];
	u8 nphy_txcal_pwr_idx[2];
	u8 nphy_rxcal_pwr_idx[2];
	u16 nphy_cal_orig_tx_gain[2];
	nphy_txgains_t nphy_cal_target_gain;
	u16 nphy_txcal_bbmult;
	u16 nphy_gmval;

	u16 nphy_saved_bbconf;

	bool nphy_gband_spurwar_en;
	bool nphy_gband_spurwar2_en;
	bool nphy_aband_spurwar_en;
	u16 nphy_rccal_value;
	u16 nphy_crsminpwr[3];
	phy_noisevar_buf_t nphy_saved_noisevars;
	bool nphy_anarxlpf_adjusted;
	bool nphy_crsminpwr_adjusted;
	bool nphy_noisevars_adjusted;

	bool nphy_rxcal_active;
	u16 radar_percal_mask;
	bool dfs_lp_buffer_nphy;

	u16 nphy_fineclockgatecontrol;

	s8 rx2tx_biasentry;

	u16 crsminpwr0;
	u16 crsminpwrl0;
	u16 crsminpwru0;
	s16 noise_crsminpwr_index;
	u16 init_gain_core1;
	u16 init_gain_core2;
	u16 init_gainb_core1;
	u16 init_gainb_core2;
	u8 aci_noise_curr_channel;
	u16 init_gain_rfseq[4];

	bool radio_is_on;

	bool nphy_sample_play_lpf_bw_ctl_ovr;

	u16 tbl_data_hi;
	u16 tbl_data_lo;
	u16 tbl_addr;

	uint tbl_save_id;
	uint tbl_save_offset;

	u8 txpwrctrl;
	s8 txpwrindex[PHY_CORE_MAX];

	u8 phycal_tempdelta;
	u32 mcs20_po;
	u32 mcs40_po;
};

typedef s32 fixed;

typedef struct _cs32 {
	fixed q;
	fixed i;
} cs32;

typedef struct radio_regs {
	u16 address;
	u32 init_a;
	u32 init_g;
	u8 do_init_a;
	u8 do_init_g;
} radio_regs_t;

typedef struct radio_20xx_regs {
	u16 address;
	u8 init;
	u8 do_init;
} radio_20xx_regs_t;

typedef struct lcnphy_radio_regs {
	u16 address;
	u8 init_a;
	u8 init_g;
	u8 do_init_a;
	u8 do_init_g;
} lcnphy_radio_regs_t;

extern lcnphy_radio_regs_t lcnphy_radio_regs_2064[];
extern lcnphy_radio_regs_t lcnphy_radio_regs_2066[];
extern radio_regs_t regs_2055[], regs_SYN_2056[], regs_TX_2056[],
    regs_RX_2056[];
extern radio_regs_t regs_SYN_2056_A1[], regs_TX_2056_A1[], regs_RX_2056_A1[];
extern radio_regs_t regs_SYN_2056_rev5[], regs_TX_2056_rev5[],
    regs_RX_2056_rev5[];
extern radio_regs_t regs_SYN_2056_rev6[], regs_TX_2056_rev6[],
    regs_RX_2056_rev6[];
extern radio_regs_t regs_SYN_2056_rev7[], regs_TX_2056_rev7[],
    regs_RX_2056_rev7[];
extern radio_regs_t regs_SYN_2056_rev8[], regs_TX_2056_rev8[],
    regs_RX_2056_rev8[];
extern radio_20xx_regs_t regs_2057_rev4[], regs_2057_rev5[], regs_2057_rev5v1[];
extern radio_20xx_regs_t regs_2057_rev7[], regs_2057_rev8[];

extern char *phy_getvar(phy_info_t *pi, const char *name);
extern int phy_getintvar(phy_info_t *pi, const char *name);
#define PHY_GETVAR(pi, name)	phy_getvar(pi, name)
#define PHY_GETINTVAR(pi, name)	phy_getintvar(pi, name)

extern u16 read_phy_reg(phy_info_t *pi, u16 addr);
extern void write_phy_reg(phy_info_t *pi, u16 addr, u16 val);
extern void and_phy_reg(phy_info_t *pi, u16 addr, u16 val);
extern void or_phy_reg(phy_info_t *pi, u16 addr, u16 val);
extern void mod_phy_reg(phy_info_t *pi, u16 addr, u16 mask, u16 val);

extern u16 read_radio_reg(phy_info_t *pi, u16 addr);
extern void or_radio_reg(phy_info_t *pi, u16 addr, u16 val);
extern void and_radio_reg(phy_info_t *pi, u16 addr, u16 val);
extern void mod_radio_reg(phy_info_t *pi, u16 addr, u16 mask,
			  u16 val);
extern void xor_radio_reg(phy_info_t *pi, u16 addr, u16 mask);

extern void write_radio_reg(phy_info_t *pi, u16 addr, u16 val);

extern void wlc_phyreg_enter(wlc_phy_t *pih);
extern void wlc_phyreg_exit(wlc_phy_t *pih);
extern void wlc_radioreg_enter(wlc_phy_t *pih);
extern void wlc_radioreg_exit(wlc_phy_t *pih);

extern void wlc_phy_read_table(phy_info_t *pi, const phytbl_info_t *ptbl_info,
			       u16 tblAddr, u16 tblDataHi,
			       u16 tblDatalo);
extern void wlc_phy_write_table(phy_info_t *pi,
				const phytbl_info_t *ptbl_info, u16 tblAddr,
				u16 tblDataHi, u16 tblDatalo);
extern void wlc_phy_table_addr(phy_info_t *pi, uint tbl_id, uint tbl_offset,
			       u16 tblAddr, u16 tblDataHi,
			       u16 tblDataLo);
extern void wlc_phy_table_data_write(phy_info_t *pi, uint width, u32 val);

extern void write_phy_channel_reg(phy_info_t *pi, uint val);
extern void wlc_phy_txpower_update_shm(phy_info_t *pi);

extern void wlc_phy_cordic(fixed theta, cs32 *val);
extern u8 wlc_phy_nbits(s32 value);
extern u32 wlc_phy_sqrt_int(u32 value);
extern void wlc_phy_compute_dB(u32 *cmplx_pwr, s8 *p_dB, u8 core);

extern uint wlc_phy_init_radio_regs_allbands(phy_info_t *pi,
					     radio_20xx_regs_t *radioregs);
extern uint wlc_phy_init_radio_regs(phy_info_t *pi, radio_regs_t *radioregs,
				    u16 core_offset);

extern void wlc_phy_txpower_ipa_upd(phy_info_t *pi);

extern void wlc_phy_do_dummy_tx(phy_info_t *pi, bool ofdm, bool pa_on);
extern void wlc_phy_papd_decode_epsilon(u32 epsilon, s32 *eps_real,
					s32 *eps_imag);

extern void wlc_phy_cal_perical_mphase_reset(phy_info_t *pi);
extern void wlc_phy_cal_perical_mphase_restart(phy_info_t *pi);

extern bool wlc_phy_attach_nphy(phy_info_t *pi);
extern bool wlc_phy_attach_lcnphy(phy_info_t *pi);

extern void wlc_phy_detach_lcnphy(phy_info_t *pi);

extern void wlc_phy_init_nphy(phy_info_t *pi);
extern void wlc_phy_init_lcnphy(phy_info_t *pi);

extern void wlc_phy_cal_init_nphy(phy_info_t *pi);
extern void wlc_phy_cal_init_lcnphy(phy_info_t *pi);

extern void wlc_phy_chanspec_set_nphy(phy_info_t *pi, chanspec_t chanspec);
extern void wlc_phy_chanspec_set_lcnphy(phy_info_t *pi, chanspec_t chanspec);
extern void wlc_phy_chanspec_set_fixup_lcnphy(phy_info_t *pi,
					      chanspec_t chanspec);
extern int wlc_phy_channel2freq(uint channel);
extern int wlc_phy_chanspec_freq2bandrange_lpssn(uint);
extern int wlc_phy_chanspec_bandrange_get(phy_info_t *, chanspec_t);

extern void wlc_lcnphy_set_tx_pwr_ctrl(phy_info_t *pi, u16 mode);
extern s8 wlc_lcnphy_get_current_tx_pwr_idx(phy_info_t *pi);

extern void wlc_phy_txpower_recalc_target_nphy(phy_info_t *pi);
extern void wlc_lcnphy_txpower_recalc_target(phy_info_t *pi);
extern void wlc_phy_txpower_recalc_target_lcnphy(phy_info_t *pi);

extern void wlc_lcnphy_set_tx_pwr_by_index(phy_info_t *pi, int index);
extern void wlc_lcnphy_tx_pu(phy_info_t *pi, bool bEnable);
extern void wlc_lcnphy_stop_tx_tone(phy_info_t *pi);
extern void wlc_lcnphy_start_tx_tone(phy_info_t *pi, s32 f_kHz,
				     u16 max_val, bool iqcalmode);

extern void wlc_phy_txpower_sromlimit_get_nphy(phy_info_t *pi, uint chan,
					       u8 *max_pwr, u8 rate_id);
extern void wlc_phy_ofdm_to_mcs_powers_nphy(u8 *power, u8 rate_mcs_start,
					    u8 rate_mcs_end,
					    u8 rate_ofdm_start);
extern void wlc_phy_mcs_to_ofdm_powers_nphy(u8 *power,
					    u8 rate_ofdm_start,
					    u8 rate_ofdm_end,
					    u8 rate_mcs_start);

extern u16 wlc_lcnphy_tempsense(phy_info_t *pi, bool mode);
extern s16 wlc_lcnphy_tempsense_new(phy_info_t *pi, bool mode);
extern s8 wlc_lcnphy_tempsense_degree(phy_info_t *pi, bool mode);
extern s8 wlc_lcnphy_vbatsense(phy_info_t *pi, bool mode);
extern void wlc_phy_carrier_suppress_lcnphy(phy_info_t *pi);
extern void wlc_lcnphy_crsuprs(phy_info_t *pi, int channel);
extern void wlc_lcnphy_epa_switch(phy_info_t *pi, bool mode);
extern void wlc_2064_vco_cal(phy_info_t *pi);

extern void wlc_phy_txpower_recalc_target(phy_info_t *pi);
extern u32 wlc_phy_qdiv_roundup(u32 dividend, u32 divisor,
				   u8 precision);

#define LCNPHY_TBL_ID_PAPDCOMPDELTATBL	0x18
#define LCNPHY_TX_POWER_TABLE_SIZE	128
#define LCNPHY_MAX_TX_POWER_INDEX	(LCNPHY_TX_POWER_TABLE_SIZE - 1)
#define LCNPHY_TBL_ID_TXPWRCTL 	0x07
#define LCNPHY_TX_PWR_CTRL_OFF	0
#define LCNPHY_TX_PWR_CTRL_SW		(0x1 << 15)
#define LCNPHY_TX_PWR_CTRL_HW         ((0x1 << 15) | \
					(0x1 << 14) | \
					(0x1 << 13))

#define LCNPHY_TX_PWR_CTRL_TEMPBASED	0xE001

extern void wlc_lcnphy_write_table(phy_info_t *pi, const phytbl_info_t *pti);
extern void wlc_lcnphy_read_table(phy_info_t *pi, phytbl_info_t *pti);
extern void wlc_lcnphy_set_tx_iqcc(phy_info_t *pi, u16 a, u16 b);
extern void wlc_lcnphy_set_tx_locc(phy_info_t *pi, u16 didq);
extern void wlc_lcnphy_get_tx_iqcc(phy_info_t *pi, u16 *a, u16 *b);
extern u16 wlc_lcnphy_get_tx_locc(phy_info_t *pi);
extern void wlc_lcnphy_get_radio_loft(phy_info_t *pi, u8 *ei0,
				      u8 *eq0, u8 *fi0, u8 *fq0);
extern void wlc_lcnphy_calib_modes(phy_info_t *pi, uint mode);
extern void wlc_lcnphy_deaf_mode(phy_info_t *pi, bool mode);
extern bool wlc_phy_tpc_isenabled_lcnphy(phy_info_t *pi);
extern void wlc_lcnphy_tx_pwr_update_npt(phy_info_t *pi);
extern s32 wlc_lcnphy_tssi2dbm(s32 tssi, s32 a1, s32 b0, s32 b1);
extern void wlc_lcnphy_get_tssi(phy_info_t *pi, s8 *ofdm_pwr,
				s8 *cck_pwr);
extern void wlc_lcnphy_tx_power_adjustment(wlc_phy_t *ppi);

extern s32 wlc_lcnphy_rx_signal_power(phy_info_t *pi, s32 gain_index);

#define NPHY_MAX_HPVGA1_INDEX		10
#define NPHY_DEF_HPVGA1_INDEXLIMIT	7

typedef struct _phy_iq_est {
	s32 iq_prod;
	u32 i_pwr;
	u32 q_pwr;
} phy_iq_est_t;

extern void wlc_phy_stay_in_carriersearch_nphy(phy_info_t *pi, bool enable);
extern void wlc_nphy_deaf_mode(phy_info_t *pi, bool mode);

#define wlc_phy_write_table_nphy(pi, pti)	wlc_phy_write_table(pi, pti, 0x72, \
	0x74, 0x73)
#define wlc_phy_read_table_nphy(pi, pti)	wlc_phy_read_table(pi, pti, 0x72, \
	0x74, 0x73)
#define wlc_nphy_table_addr(pi, id, off)	wlc_phy_table_addr((pi), (id), (off), \
	0x72, 0x74, 0x73)
#define wlc_nphy_table_data_write(pi, w, v)	wlc_phy_table_data_write((pi), (w), (v))

extern void wlc_phy_table_read_nphy(phy_info_t *pi, u32, u32 l, u32 o,
				    u32 w, void *d);
extern void wlc_phy_table_write_nphy(phy_info_t *pi, u32, u32, u32,
				     u32, const void *);

#define	PHY_IPA(pi) \
	((pi->ipa2g_on && CHSPEC_IS2G(pi->radio_chanspec)) || \
	 (pi->ipa5g_on && CHSPEC_IS5G(pi->radio_chanspec)))

#define WLC_PHY_WAR_PR51571(pi) \
	if ((BUSTYPE((pi)->sh->bustype) == PCI_BUS) && NREV_LT((pi)->pubpi.phy_rev, 3)) \
		(void)R_REG((pi)->sh->osh, &(pi)->regs->maccontrol)

extern void wlc_phy_cal_perical_nphy_run(phy_info_t *pi, u8 caltype);
extern void wlc_phy_aci_reset_nphy(phy_info_t *pi);
extern void wlc_phy_pa_override_nphy(phy_info_t *pi, bool en);

extern u8 wlc_phy_get_chan_freq_range_nphy(phy_info_t *pi, uint chan);
extern void wlc_phy_switch_radio_nphy(phy_info_t *pi, bool on);

extern void wlc_phy_stf_chain_upd_nphy(phy_info_t *pi);

extern void wlc_phy_force_rfseq_nphy(phy_info_t *pi, u8 cmd);
extern s16 wlc_phy_tempsense_nphy(phy_info_t *pi);

extern u16 wlc_phy_classifier_nphy(phy_info_t *pi, u16 mask, u16 val);

extern void wlc_phy_rx_iq_est_nphy(phy_info_t *pi, phy_iq_est_t *est,
				   u16 num_samps, u8 wait_time,
				   u8 wait_for_crs);

extern void wlc_phy_rx_iq_coeffs_nphy(phy_info_t *pi, u8 write,
				      nphy_iq_comp_t *comp);
extern void wlc_phy_aci_and_noise_reduction_nphy(phy_info_t *pi);

extern void wlc_phy_rxcore_setstate_nphy(wlc_phy_t *pih, u8 rxcore_bitmask);
extern u8 wlc_phy_rxcore_getstate_nphy(wlc_phy_t *pih);

extern void wlc_phy_txpwrctrl_enable_nphy(phy_info_t *pi, u8 ctrl_type);
extern void wlc_phy_txpwr_fixpower_nphy(phy_info_t *pi);
extern void wlc_phy_txpwr_apply_nphy(phy_info_t *pi);
extern void wlc_phy_txpwr_papd_cal_nphy(phy_info_t *pi);
extern u16 wlc_phy_txpwr_idx_get_nphy(phy_info_t *pi);

extern nphy_txgains_t wlc_phy_get_tx_gain_nphy(phy_info_t *pi);
extern int wlc_phy_cal_txiqlo_nphy(phy_info_t *pi, nphy_txgains_t target_gain,
				   bool full, bool m);
extern int wlc_phy_cal_rxiq_nphy(phy_info_t *pi, nphy_txgains_t target_gain,
				 u8 type, bool d);
extern void wlc_phy_txpwr_index_nphy(phy_info_t *pi, u8 core_mask,
				     s8 txpwrindex, bool res);
extern void wlc_phy_rssisel_nphy(phy_info_t *pi, u8 core, u8 rssi_type);
extern int wlc_phy_poll_rssi_nphy(phy_info_t *pi, u8 rssi_type,
				  s32 *rssi_buf, u8 nsamps);
extern void wlc_phy_rssi_cal_nphy(phy_info_t *pi);
extern int wlc_phy_aci_scan_nphy(phy_info_t *pi);
extern void wlc_phy_cal_txgainctrl_nphy(phy_info_t *pi, s32 dBm_targetpower,
					bool debug);
extern int wlc_phy_tx_tone_nphy(phy_info_t *pi, u32 f_kHz, u16 max_val,
				u8 mode, u8, bool);
extern void wlc_phy_stopplayback_nphy(phy_info_t *pi);
extern void wlc_phy_est_tonepwr_nphy(phy_info_t *pi, s32 *qdBm_pwrbuf,
				     u8 num_samps);
extern void wlc_phy_radio205x_vcocal_nphy(phy_info_t *pi);

extern int wlc_phy_rssi_compute_nphy(phy_info_t *pi, wlc_d11rxhdr_t *wlc_rxh);

#define NPHY_TESTPATTERN_BPHY_EVM   0
#define NPHY_TESTPATTERN_BPHY_RFCS  1

extern void wlc_phy_nphy_tkip_rifs_war(phy_info_t *pi, u8 rifs);

void wlc_phy_get_pwrdet_offsets(phy_info_t *pi, s8 *cckoffset,
				s8 *ofdmoffset);
extern s8 wlc_phy_upd_rssi_offset(phy_info_t *pi, s8 rssi,
				    chanspec_t chanspec);

extern bool wlc_phy_n_txpower_ipa_ison(phy_info_t *pih);
#endif				/* _wlc_phy_int_h_ */
