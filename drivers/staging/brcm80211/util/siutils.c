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

#include <linux/kernel.h>
#include <linux/string.h>
#include <bcmdefs.h>
#include <osl.h>
#include <linuxver.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmdevs.h>
#include <hndsoc.h>
#include <sbchipc.h>
#include <pci_core.h>
#include <pcie_core.h>
#include <nicpci.h>
#include <bcmnvram.h>
#include <bcmsrom.h>
#include <pcicfg.h>
#include <sbsocram.h>
#ifdef BCMSDIO
#include <bcmsdh.h>
#include <sdio.h>
#include <sbsdio.h>
#include <sbhnddma.h>
#include <sbsdpcmdev.h>
#include <bcmsdpcm.h>
#endif				/* BCMSDIO */
#include <hndpmu.h>

/* this file now contains only definitions for sb functions, only necessary
*for devices using Sonics backplanes (bcm4329)
*/

/* if an amba SDIO device is supported, please further restrict the inclusion
 * of this file
 */
#ifdef BCMSDIO
#include "siutils_priv.h"
#endif

/* local prototypes */
static si_info_t *si_doattach(si_info_t *sii, uint devid, osl_t *osh,
			      void *regs, uint bustype, void *sdh, char **vars,
			      uint *varsz);
static bool si_buscore_prep(si_info_t *sii, uint bustype, uint devid,
			    void *sdh);
static bool si_buscore_setup(si_info_t *sii, chipcregs_t *cc, uint bustype,
			     u32 savewin, uint *origidx, void *regs);
static void si_nvram_process(si_info_t *sii, char *pvars);

/* dev path concatenation util */
static char *si_devpathvar(si_t *sih, char *var, int len, const char *name);
static bool _si_clkctl_cc(si_info_t *sii, uint mode);
static bool si_ispcie(si_info_t *sii);
static uint socram_banksize(si_info_t *sii, sbsocramregs_t *r,
					u8 idx, u8 mtype);

/* global variable to indicate reservation/release of gpio's */
static u32 si_gpioreservation;

/*
 * Allocate a si handle.
 * devid - pci device id (used to determine chip#)
 * osh - opaque OS handle
 * regs - virtual address of initial core registers
 * bustype - pci/sb/sdio/etc
 * vars - pointer to a pointer area for "environment" variables
 * varsz - pointer to int to return the size of the vars
 */
si_t *si_attach(uint devid, osl_t *osh, void *regs, uint bustype, void *sdh,
		char **vars, uint *varsz)
{
	si_info_t *sii;

	/* alloc si_info_t */
	sii = kmalloc(sizeof(si_info_t), GFP_ATOMIC);
	if (sii == NULL) {
		SI_ERROR(("si_attach: malloc failed!\n"));
		return NULL;
	}

	if (si_doattach(sii, devid, osh, regs, bustype, sdh, vars, varsz) ==
	    NULL) {
		kfree(sii);
		return NULL;
	}
	sii->vars = vars ? *vars : NULL;
	sii->varsz = varsz ? *varsz : 0;

	return (si_t *) sii;
}

/* global kernel resource */
static si_info_t ksii;

static bool si_buscore_prep(si_info_t *sii, uint bustype, uint devid,
			    void *sdh)
{

#ifndef BRCM_FULLMAC
	/* kludge to enable the clock on the 4306 which lacks a slowclock */
	if (BUSTYPE(bustype) == PCI_BUS && !si_ispcie(sii))
		si_clkctl_xtal(&sii->pub, XTAL | PLL, ON);
#endif

#if defined(BCMSDIO)
	if (BUSTYPE(bustype) == SDIO_BUS) {
		int err;
		u8 clkset;

		/* Try forcing SDIO core to do ALPAvail request only */
		clkset = SBSDIO_FORCE_HW_CLKREQ_OFF | SBSDIO_ALP_AVAIL_REQ;
		bcmsdh_cfg_write(sdh, SDIO_FUNC_1, SBSDIO_FUNC1_CHIPCLKCSR,
				 clkset, &err);
		if (!err) {
			u8 clkval;

			/* If register supported, wait for ALPAvail and then force ALP */
			clkval =
			    bcmsdh_cfg_read(sdh, SDIO_FUNC_1,
					    SBSDIO_FUNC1_CHIPCLKCSR, NULL);
			if ((clkval & ~SBSDIO_AVBITS) == clkset) {
				SPINWAIT(((clkval =
					   bcmsdh_cfg_read(sdh, SDIO_FUNC_1,
							   SBSDIO_FUNC1_CHIPCLKCSR,
							   NULL)),
					  !SBSDIO_ALPAV(clkval)),
					 PMU_MAX_TRANSITION_DLY);
				if (!SBSDIO_ALPAV(clkval)) {
					SI_ERROR(("timeout on ALPAV wait, clkval 0x%02x\n", clkval));
					return false;
				}
				clkset =
				    SBSDIO_FORCE_HW_CLKREQ_OFF |
				    SBSDIO_FORCE_ALP;
				bcmsdh_cfg_write(sdh, SDIO_FUNC_1,
						 SBSDIO_FUNC1_CHIPCLKCSR,
						 clkset, &err);
				udelay(65);
			}
		}

		/* Also, disable the extra SDIO pull-ups */
		bcmsdh_cfg_write(sdh, SDIO_FUNC_1, SBSDIO_FUNC1_SDIOPULLUP, 0,
				 NULL);
	}
#endif				/* defined(BCMSDIO) */

	return true;
}

static bool si_buscore_setup(si_info_t *sii, chipcregs_t *cc, uint bustype,
			     u32 savewin, uint *origidx, void *regs)
{
	bool pci, pcie;
	uint i;
	uint pciidx, pcieidx, pcirev, pcierev;

	cc = si_setcoreidx(&sii->pub, SI_CC_IDX);
	ASSERT(cc);

	/* get chipcommon rev */
	sii->pub.ccrev = (int)si_corerev(&sii->pub);

	/* get chipcommon chipstatus */
	if (sii->pub.ccrev >= 11)
		sii->pub.chipst = R_REG(sii->osh, &cc->chipstatus);

	/* get chipcommon capabilites */
	sii->pub.cccaps = R_REG(sii->osh, &cc->capabilities);
	/* get chipcommon extended capabilities */

#ifndef BRCM_FULLMAC
	if (sii->pub.ccrev >= 35)
		sii->pub.cccaps_ext = R_REG(sii->osh, &cc->capabilities_ext);
#endif
	/* get pmu rev and caps */
	if (sii->pub.cccaps & CC_CAP_PMU) {
		sii->pub.pmucaps = R_REG(sii->osh, &cc->pmucapabilities);
		sii->pub.pmurev = sii->pub.pmucaps & PCAP_REV_MASK;
	}

	/*
	   SI_MSG(("Chipc: rev %d, caps 0x%x, chipst 0x%x pmurev %d, pmucaps 0x%x\n",
	   sii->pub.ccrev, sii->pub.cccaps, sii->pub.chipst, sii->pub.pmurev,
	   sii->pub.pmucaps));
	 */

	/* figure out bus/orignal core idx */
	sii->pub.buscoretype = NODEV_CORE_ID;
	sii->pub.buscorerev = NOREV;
	sii->pub.buscoreidx = BADIDX;

	pci = pcie = false;
	pcirev = pcierev = NOREV;
	pciidx = pcieidx = BADIDX;

	for (i = 0; i < sii->numcores; i++) {
		uint cid, crev;

		si_setcoreidx(&sii->pub, i);
		cid = si_coreid(&sii->pub);
		crev = si_corerev(&sii->pub);

		/* Display cores found */
		SI_VMSG(("CORE[%d]: id 0x%x rev %d base 0x%x regs 0x%p\n",
			 i, cid, crev, sii->coresba[i], sii->regs[i]));

		if (BUSTYPE(bustype) == PCI_BUS) {
			if (cid == PCI_CORE_ID) {
				pciidx = i;
				pcirev = crev;
				pci = true;
			} else if (cid == PCIE_CORE_ID) {
				pcieidx = i;
				pcierev = crev;
				pcie = true;
			}
		}
#ifdef BCMSDIO
		else if (((BUSTYPE(bustype) == SDIO_BUS) ||
			  (BUSTYPE(bustype) == SPI_BUS)) &&
			 ((cid == PCMCIA_CORE_ID) || (cid == SDIOD_CORE_ID))) {
			sii->pub.buscorerev = crev;
			sii->pub.buscoretype = cid;
			sii->pub.buscoreidx = i;
		}
#endif				/* BCMSDIO */

		/* find the core idx before entering this func. */
		if ((savewin && (savewin == sii->coresba[i])) ||
		    (regs == sii->regs[i]))
			*origidx = i;
	}

#ifdef BRCM_FULLMAC
	SI_MSG(("Buscore id/type/rev %d/0x%x/%d\n", sii->pub.buscoreidx,
		sii->pub.buscoretype, sii->pub.buscorerev));

	/* Make sure any on-chip ARM is off (in case strapping is wrong),
	* or downloaded code was
	* already running.
	*/
	if ((BUSTYPE(bustype) == SDIO_BUS) || (BUSTYPE(bustype) == SPI_BUS)) {
		if (si_setcore(&sii->pub, ARM7S_CORE_ID, 0) ||
			si_setcore(&sii->pub, ARMCM3_CORE_ID, 0))
			si_core_disable(&sii->pub, 0);
	}
#else
	if (pci && pcie) {
		if (si_ispcie(sii))
			pci = false;
		else
			pcie = false;
	}
	if (pci) {
		sii->pub.buscoretype = PCI_CORE_ID;
		sii->pub.buscorerev = pcirev;
		sii->pub.buscoreidx = pciidx;
	} else if (pcie) {
		sii->pub.buscoretype = PCIE_CORE_ID;
		sii->pub.buscorerev = pcierev;
		sii->pub.buscoreidx = pcieidx;
	}

	SI_VMSG(("Buscore id/type/rev %d/0x%x/%d\n", sii->pub.buscoreidx,
		 sii->pub.buscoretype, sii->pub.buscorerev));

	/* fixup necessary chip/core configurations */
	if (BUSTYPE(sii->pub.bustype) == PCI_BUS) {
		if (SI_FAST(sii)) {
			if (!sii->pch) {
				sii->pch = (void *)pcicore_init(
					&sii->pub, sii->osh,
					(void *)PCIEREGS(sii));
				if (sii->pch == NULL)
					return false;
			}
		}
		if (si_pci_fixcfg(&sii->pub)) {
			SI_ERROR(("si_doattach: sb_pci_fixcfg failed\n"));
			return false;
		}
	}
#endif
	/* return to the original core */
	si_setcoreidx(&sii->pub, *origidx);

	return true;
}

static __used void si_nvram_process(si_info_t *sii, char *pvars)
{
	uint w = 0;

	/* get boardtype and boardrev */
	switch (BUSTYPE(sii->pub.bustype)) {
	case PCI_BUS:
		/* do a pci config read to get subsystem id and subvendor id */
		w = OSL_PCI_READ_CONFIG(sii->osh, PCI_CFG_SVID, sizeof(u32));
		/* Let nvram variables override subsystem Vend/ID */
		sii->pub.boardvendor = (u16)si_getdevpathintvar(&sii->pub,
			"boardvendor");
		if (sii->pub.boardvendor == 0)
			sii->pub.boardvendor = w & 0xffff;
		else
			SI_ERROR(("Overriding boardvendor: 0x%x instead of 0x%x\n", sii->pub.boardvendor, w & 0xffff));
		sii->pub.boardtype = (u16)si_getdevpathintvar(&sii->pub,
			"boardtype");
		if (sii->pub.boardtype == 0)
			sii->pub.boardtype = (w >> 16) & 0xffff;
		else
			SI_ERROR(("Overriding boardtype: 0x%x instead of 0x%x\n", sii->pub.boardtype, (w >> 16) & 0xffff));
		break;

#ifdef BCMSDIO
	case SDIO_BUS:
#endif
		sii->pub.boardvendor = getintvar(pvars, "manfid");
		sii->pub.boardtype = getintvar(pvars, "prodid");
		break;

#ifdef BCMSDIO
	case SPI_BUS:
		sii->pub.boardvendor = VENDOR_BROADCOM;
		sii->pub.boardtype = SPI_BOARD;
		break;
#endif

	case SI_BUS:
	case JTAG_BUS:
		sii->pub.boardvendor = VENDOR_BROADCOM;
		sii->pub.boardtype = getintvar(pvars, "prodid");
		if (pvars == NULL || (sii->pub.boardtype == 0)) {
			sii->pub.boardtype = getintvar(NULL, "boardtype");
			if (sii->pub.boardtype == 0)
				sii->pub.boardtype = 0xffff;
		}
		break;
	}

	if (sii->pub.boardtype == 0) {
		SI_ERROR(("si_doattach: unknown board type\n"));
		ASSERT(sii->pub.boardtype);
	}

	sii->pub.boardflags = getintvar(pvars, "boardflags");
}

/* this is will make Sonics calls directly, since Sonics is no longer supported in the Si abstraction */
/* this has been customized for the bcm 4329 ONLY */
#ifdef BCMSDIO
static si_info_t *si_doattach(si_info_t *sii, uint devid, osl_t *osh,
			      void *regs, uint bustype, void *sdh,
			      char **vars, uint *varsz)
{
	struct si_pub *sih = &sii->pub;
	u32 w, savewin;
	chipcregs_t *cc;
	char *pvars = NULL;
	uint origidx;

	ASSERT(GOODREGS(regs));

	bzero((unsigned char *) sii, sizeof(si_info_t));

	savewin = 0;

	sih->buscoreidx = BADIDX;

	sii->curmap = regs;
	sii->sdh = sdh;
	sii->osh = osh;

	/* find Chipcommon address */
	cc = (chipcregs_t *) sii->curmap;
	sih->bustype = bustype;

	if (bustype != BUSTYPE(bustype)) {
		SI_ERROR(("si_doattach: bus type %d does not match configured bus type %d\n", bustype, BUSTYPE(bustype)));
		return NULL;
	}

	/* bus/core/clk setup for register access */
	if (!si_buscore_prep(sii, bustype, devid, sdh)) {
		SI_ERROR(("si_doattach: si_core_clk_prep failed %d\n",
			  bustype));
		return NULL;
	}

	/* ChipID recognition.
	 *   We assume we can read chipid at offset 0 from the regs arg.
	 *   If we add other chiptypes (or if we need to support old sdio hosts w/o chipcommon),
	 *   some way of recognizing them needs to be added here.
	 */
	w = R_REG(osh, &cc->chipid);
	sih->socitype = (w & CID_TYPE_MASK) >> CID_TYPE_SHIFT;
	/* Might as wll fill in chip id rev & pkg */
	sih->chip = w & CID_ID_MASK;
	sih->chiprev = (w & CID_REV_MASK) >> CID_REV_SHIFT;
	sih->chippkg = (w & CID_PKG_MASK) >> CID_PKG_SHIFT;

	if ((CHIPID(sih->chip) == BCM4329_CHIP_ID) &&
		(sih->chippkg != BCM4329_289PIN_PKG_ID))
			sih->chippkg = BCM4329_182PIN_PKG_ID;

	sih->issim = IS_SIM(sih->chippkg);

	/* scan for cores */
	/* SI_MSG(("Found chip type SB (0x%08x)\n", w)); */
	sb_scan(&sii->pub, regs, devid);

	/* no cores found, bail out */
	if (sii->numcores == 0) {
		SI_ERROR(("si_doattach: could not find any cores\n"));
		return NULL;
	}
	/* bus/core/clk setup */
	origidx = SI_CC_IDX;
	if (!si_buscore_setup(sii, cc, bustype, savewin, &origidx, regs)) {
		SI_ERROR(("si_doattach: si_buscore_setup failed\n"));
		goto exit;
	}

#ifdef BRCM_FULLMAC
	pvars = NULL;
#else
	/* Init nvram from flash if it exists */
	nvram_init((void *)&(sii->pub));

	/* Init nvram from sprom/otp if they exist */
	if (srom_var_init
	    (&sii->pub, BUSTYPE(bustype), regs, sii->osh, vars, varsz)) {
		SI_ERROR(("si_doattach: srom_var_init failed: bad srom\n"));
		goto exit;
	}
	pvars = vars ? *vars : NULL;
	si_nvram_process(sii, pvars);
#endif

	/* === NVRAM, clock is ready === */

#ifdef BRCM_FULLMAC
	if (sii->pub.ccrev >= 20) {
#endif
	cc = (chipcregs_t *) si_setcore(sih, CC_CORE_ID, 0);
	W_REG(osh, &cc->gpiopullup, 0);
	W_REG(osh, &cc->gpiopulldown, 0);
	sb_setcoreidx(sih, origidx);
#ifdef BRCM_FULLMAC
	}
#endif

#ifndef BRCM_FULLMAC
	/* PMU specific initializations */
	if (PMUCTL_ENAB(sih)) {
		u32 xtalfreq;
		si_pmu_init(sih, sii->osh);
		si_pmu_chip_init(sih, sii->osh);
		xtalfreq = getintvar(pvars, "xtalfreq");
		/* If xtalfreq var not available, try to measure it */
		if (xtalfreq == 0)
			xtalfreq = si_pmu_measure_alpclk(sih, sii->osh);
		si_pmu_pll_init(sih, sii->osh, xtalfreq);
		si_pmu_res_init(sih, sii->osh);
		si_pmu_swreg_init(sih, sii->osh);
	}

	/* setup the GPIO based LED powersave register */
	w = getintvar(pvars, "leddc");
	if (w == 0)
		w = DEFAULT_GPIOTIMERVAL;
	sb_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t, gpiotimerval), ~0, w);

#ifdef BCMDBG
	/* clear any previous epidiag-induced target abort */
	sb_taclear(sih, false);
#endif				/* BCMDBG */
#endif

	return sii;

 exit:
	return NULL;
}

#else				/* BCMSDIO */
static si_info_t *si_doattach(si_info_t *sii, uint devid, osl_t *osh,
			      void *regs, uint bustype, void *sdh,
			      char **vars, uint *varsz)
{
	struct si_pub *sih = &sii->pub;
	u32 w, savewin;
	chipcregs_t *cc;
	char *pvars = NULL;
	uint origidx;

	ASSERT(GOODREGS(regs));

	bzero((unsigned char *) sii, sizeof(si_info_t));

	savewin = 0;

	sih->buscoreidx = BADIDX;

	sii->curmap = regs;
	sii->sdh = sdh;
	sii->osh = osh;

	/* check to see if we are a si core mimic'ing a pci core */
	if ((bustype == PCI_BUS) &&
	    (OSL_PCI_READ_CONFIG(sii->osh, PCI_SPROM_CONTROL, sizeof(u32)) ==
	     0xffffffff)) {
		SI_ERROR(("%s: incoming bus is PCI but it's a lie, switching to SI " "devid:0x%x\n", __func__, devid));
		bustype = SI_BUS;
	}

	/* find Chipcommon address */
	if (bustype == PCI_BUS) {
		savewin =
		    OSL_PCI_READ_CONFIG(sii->osh, PCI_BAR0_WIN, sizeof(u32));
		if (!GOODCOREADDR(savewin, SI_ENUM_BASE))
			savewin = SI_ENUM_BASE;
		OSL_PCI_WRITE_CONFIG(sii->osh, PCI_BAR0_WIN, 4, SI_ENUM_BASE);
		cc = (chipcregs_t *) regs;
	} else {
		cc = (chipcregs_t *) REG_MAP(SI_ENUM_BASE, SI_CORE_SIZE);
	}

	sih->bustype = bustype;
	if (bustype != BUSTYPE(bustype)) {
		SI_ERROR(("si_doattach: bus type %d does not match configured bus type %d\n", bustype, BUSTYPE(bustype)));
		return NULL;
	}

	/* bus/core/clk setup for register access */
	if (!si_buscore_prep(sii, bustype, devid, sdh)) {
		SI_ERROR(("si_doattach: si_core_clk_prep failed %d\n",
			  bustype));
		return NULL;
	}

	/* ChipID recognition.
	 *   We assume we can read chipid at offset 0 from the regs arg.
	 *   If we add other chiptypes (or if we need to support old sdio hosts w/o chipcommon),
	 *   some way of recognizing them needs to be added here.
	 */
	w = R_REG(osh, &cc->chipid);
	sih->socitype = (w & CID_TYPE_MASK) >> CID_TYPE_SHIFT;
	/* Might as wll fill in chip id rev & pkg */
	sih->chip = w & CID_ID_MASK;
	sih->chiprev = (w & CID_REV_MASK) >> CID_REV_SHIFT;
	sih->chippkg = (w & CID_PKG_MASK) >> CID_PKG_SHIFT;

	sih->issim = IS_SIM(sih->chippkg);

	/* scan for cores */
	if (CHIPTYPE(sii->pub.socitype) == SOCI_AI) {
		SI_MSG(("Found chip type AI (0x%08x)\n", w));
		/* pass chipc address instead of original core base */
		ai_scan(&sii->pub, (void *)cc, devid);
	} else {
		SI_ERROR(("Found chip of unknown type (0x%08x)\n", w));
		return NULL;
	}
	/* no cores found, bail out */
	if (sii->numcores == 0) {
		SI_ERROR(("si_doattach: could not find any cores\n"));
		return NULL;
	}
	/* bus/core/clk setup */
	origidx = SI_CC_IDX;
	if (!si_buscore_setup(sii, cc, bustype, savewin, &origidx, regs)) {
		SI_ERROR(("si_doattach: si_buscore_setup failed\n"));
		goto exit;
	}

	/* assume current core is CC */
	if ((sii->pub.ccrev == 0x25)
	    &&
	    ((CHIPID(sih->chip) == BCM43236_CHIP_ID
	      || CHIPID(sih->chip) == BCM43235_CHIP_ID
	      || CHIPID(sih->chip) == BCM43238_CHIP_ID)
	     && (CHIPREV(sii->pub.chiprev) <= 2))) {

		if ((cc->chipstatus & CST43236_BP_CLK) != 0) {
			uint clkdiv;
			clkdiv = R_REG(osh, &cc->clkdiv);
			/* otp_clk_div is even number, 120/14 < 9mhz */
			clkdiv = (clkdiv & ~CLKD_OTP) | (14 << CLKD_OTP_SHIFT);
			W_REG(osh, &cc->clkdiv, clkdiv);
			SI_ERROR(("%s: set clkdiv to %x\n", __func__, clkdiv));
		}
		udelay(10);
	}

	/* Init nvram from flash if it exists */
	nvram_init((void *)&(sii->pub));

	/* Init nvram from sprom/otp if they exist */
	if (srom_var_init
	    (&sii->pub, BUSTYPE(bustype), regs, sii->osh, vars, varsz)) {
		SI_ERROR(("si_doattach: srom_var_init failed: bad srom\n"));
		goto exit;
	}
	pvars = vars ? *vars : NULL;
	si_nvram_process(sii, pvars);

	/* === NVRAM, clock is ready === */
	cc = (chipcregs_t *) si_setcore(sih, CC_CORE_ID, 0);
	W_REG(osh, &cc->gpiopullup, 0);
	W_REG(osh, &cc->gpiopulldown, 0);
	si_setcoreidx(sih, origidx);

	/* PMU specific initializations */
	if (PMUCTL_ENAB(sih)) {
		u32 xtalfreq;
		si_pmu_init(sih, sii->osh);
		si_pmu_chip_init(sih, sii->osh);
		xtalfreq = getintvar(pvars, "xtalfreq");
		/* If xtalfreq var not available, try to measure it */
		if (xtalfreq == 0)
			xtalfreq = si_pmu_measure_alpclk(sih, sii->osh);
		si_pmu_pll_init(sih, sii->osh, xtalfreq);
		si_pmu_res_init(sih, sii->osh);
		si_pmu_swreg_init(sih, sii->osh);
	}

	/* setup the GPIO based LED powersave register */
	w = getintvar(pvars, "leddc");
	if (w == 0)
		w = DEFAULT_GPIOTIMERVAL;
	si_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t, gpiotimerval), ~0, w);

	if (PCIE(sii)) {
		ASSERT(sii->pch != NULL);
		pcicore_attach(sii->pch, pvars, SI_DOATTACH);
	}

	if ((CHIPID(sih->chip) == BCM43224_CHIP_ID) ||
	    (CHIPID(sih->chip) == BCM43421_CHIP_ID)) {
		/* enable 12 mA drive strenth for 43224 and set chipControl register bit 15 */
		if (CHIPREV(sih->chiprev) == 0) {
			SI_MSG(("Applying 43224A0 WARs\n"));
			si_corereg(sih, SI_CC_IDX,
				   offsetof(chipcregs_t, chipcontrol),
				   CCTRL43224_GPIO_TOGGLE,
				   CCTRL43224_GPIO_TOGGLE);
			si_pmu_chipcontrol(sih, 0, CCTRL_43224A0_12MA_LED_DRIVE,
					   CCTRL_43224A0_12MA_LED_DRIVE);
		}
		if (CHIPREV(sih->chiprev) >= 1) {
			SI_MSG(("Applying 43224B0+ WARs\n"));
			si_pmu_chipcontrol(sih, 0, CCTRL_43224B0_12MA_LED_DRIVE,
					   CCTRL_43224B0_12MA_LED_DRIVE);
		}
	}

	if (CHIPID(sih->chip) == BCM4313_CHIP_ID) {
		/* enable 12 mA drive strenth for 4313 and set chipControl register bit 1 */
		SI_MSG(("Applying 4313 WARs\n"));
		si_pmu_chipcontrol(sih, 0, CCTRL_4313_12MA_LED_DRIVE,
				   CCTRL_4313_12MA_LED_DRIVE);
	}

	if (CHIPID(sih->chip) == BCM4331_CHIP_ID) {
		/* Enable Ext PA lines depending on chip package option */
		si_chipcontrl_epa4331(sih, true);
	}

	return sii;
 exit:
	if (BUSTYPE(sih->bustype) == PCI_BUS) {
		if (sii->pch)
			pcicore_deinit(sii->pch);
		sii->pch = NULL;
	}

	return NULL;
}
#endif				/* BCMSDIO */

/* may be called with core in reset */
void si_detach(si_t *sih)
{
	si_info_t *sii;
	uint idx;

	struct si_pub *si_local = NULL;
	bcopy(&sih, &si_local, sizeof(si_t **));

	sii = SI_INFO(sih);

	if (sii == NULL)
		return;

	if (BUSTYPE(sih->bustype) == SI_BUS)
		for (idx = 0; idx < SI_MAXCORES; idx++)
			if (sii->regs[idx]) {
				REG_UNMAP(sii->regs[idx]);
				sii->regs[idx] = NULL;
			}

#ifndef BRCM_FULLMAC
	nvram_exit((void *)si_local);	/* free up nvram buffers */

	if (BUSTYPE(sih->bustype) == PCI_BUS) {
		if (sii->pch)
			pcicore_deinit(sii->pch);
		sii->pch = NULL;
	}
#endif
#if !defined(BCMBUSTYPE) || (BCMBUSTYPE == SI_BUS)
	if (sii != &ksii)
#endif				/* !BCMBUSTYPE || (BCMBUSTYPE == SI_BUS) */
		kfree(sii);
}

void *si_osh(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);
	return sii->osh;
}

/* register driver interrupt disabling and restoring callback functions */
void
si_register_intr_callback(si_t *sih, void *intrsoff_fn, void *intrsrestore_fn,
			  void *intrsenabled_fn, void *intr_arg)
{
	si_info_t *sii;

	sii = SI_INFO(sih);
	sii->intr_arg = intr_arg;
	sii->intrsoff_fn = (si_intrsoff_t) intrsoff_fn;
	sii->intrsrestore_fn = (si_intrsrestore_t) intrsrestore_fn;
	sii->intrsenabled_fn = (si_intrsenabled_t) intrsenabled_fn;
	/* save current core id.  when this function called, the current core
	 * must be the core which provides driver functions(il, et, wl, etc.)
	 */
	sii->dev_coreid = sii->coreid[sii->curidx];
}

void si_deregister_intr_callback(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);
	sii->intrsoff_fn = NULL;
}

uint si_flag(si_t *sih)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_flag(sih);
	else {
		ASSERT(0);
		return 0;
	}
}

void si_setint(si_t *sih, int siflag)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		ai_setint(sih, siflag);
	else
		ASSERT(0);
}

#ifndef BCMSDIO
uint si_coreid(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);
	return sii->coreid[sii->curidx];
}
#endif

uint si_coreidx(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);
	return sii->curidx;
}

bool si_backplane64(si_t *sih)
{
	return (sih->cccaps & CC_CAP_BKPLN64) != 0;
}

#ifndef BCMSDIO
uint si_corerev(si_t *sih)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_corerev(sih);
	else {
		ASSERT(0);
		return 0;
	}
}
#endif

/* return index of coreid or BADIDX if not found */
uint si_findcoreidx(si_t *sih, uint coreid, uint coreunit)
{
	si_info_t *sii;
	uint found;
	uint i;

	sii = SI_INFO(sih);

	found = 0;

	for (i = 0; i < sii->numcores; i++)
		if (sii->coreid[i] == coreid) {
			if (found == coreunit)
				return i;
			found++;
		}

	return BADIDX;
}

/*
 * This function changes logical "focus" to the indicated core;
 * must be called with interrupts off.
 * Moreover, callers should keep interrupts off during switching out of and back to d11 core
 */
void *si_setcore(si_t *sih, uint coreid, uint coreunit)
{
	uint idx;

	idx = si_findcoreidx(sih, coreid, coreunit);
	if (!GOODIDX(idx))
		return NULL;

	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_setcoreidx(sih, idx);
	else {
#ifdef BCMSDIO
		return sb_setcoreidx(sih, idx);
#else
		ASSERT(0);
		return NULL;
#endif
	}
}

#ifndef BCMSDIO
void *si_setcoreidx(si_t *sih, uint coreidx)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_setcoreidx(sih, coreidx);
	else {
		ASSERT(0);
		return NULL;
	}
}
#endif

/* Turn off interrupt as required by sb_setcore, before switch core */
void *si_switch_core(si_t *sih, uint coreid, uint *origidx, uint *intr_val)
{
	void *cc;
	si_info_t *sii;

	sii = SI_INFO(sih);

	if (SI_FAST(sii)) {
		/* Overloading the origidx variable to remember the coreid,
		 * this works because the core ids cannot be confused with
		 * core indices.
		 */
		*origidx = coreid;
		if (coreid == CC_CORE_ID)
			return (void *)CCREGS_FAST(sii);
		else if (coreid == sih->buscoretype)
			return (void *)PCIEREGS(sii);
	}
	INTR_OFF(sii, *intr_val);
	*origidx = sii->curidx;
	cc = si_setcore(sih, coreid, 0);
	ASSERT(cc != NULL);

	return cc;
}

/* restore coreidx and restore interrupt */
void si_restore_core(si_t *sih, uint coreid, uint intr_val)
{
	si_info_t *sii;

	sii = SI_INFO(sih);
	if (SI_FAST(sii)
	    && ((coreid == CC_CORE_ID) || (coreid == sih->buscoretype)))
		return;

	si_setcoreidx(sih, coreid);
	INTR_RESTORE(sii, intr_val);
}

u32 si_core_cflags(si_t *sih, u32 mask, u32 val)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_core_cflags(sih, mask, val);
	else {
		ASSERT(0);
		return 0;
	}
}

u32 si_core_sflags(si_t *sih, u32 mask, u32 val)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_core_sflags(sih, mask, val);
	else {
		ASSERT(0);
		return 0;
	}
}

bool si_iscoreup(si_t *sih)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_iscoreup(sih);
	else {
#ifdef BCMSDIO
		return sb_iscoreup(sih);
#else
		ASSERT(0);
		return false;
#endif
	}
}

void si_write_wrapperreg(si_t *sih, u32 offset, u32 val)
{
	/* only for 4319, no requirement for SOCI_SB */
	if (CHIPTYPE(sih->socitype) == SOCI_AI) {
		ai_write_wrap_reg(sih, offset, val);
	}
}

uint si_corereg(si_t *sih, uint coreidx, uint regoff, uint mask, uint val)
{

	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		return ai_corereg(sih, coreidx, regoff, mask, val);
	else {
#ifdef BCMSDIO
		return sb_corereg(sih, coreidx, regoff, mask, val);
#else
		ASSERT(0);
		return 0;
#endif
	}
}

void si_core_disable(si_t *sih, u32 bits)
{

	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		ai_core_disable(sih, bits);
#ifdef BCMSDIO
	else
		sb_core_disable(sih, bits);
#endif
}

void si_core_reset(si_t *sih, u32 bits, u32 resetbits)
{
	if (CHIPTYPE(sih->socitype) == SOCI_AI)
		ai_core_reset(sih, bits, resetbits);
#ifdef BCMSDIO
	else
		sb_core_reset(sih, bits, resetbits);
#endif
}

u32 si_alp_clock(si_t *sih)
{
	if (PMUCTL_ENAB(sih))
		return si_pmu_alp_clock(sih, si_osh(sih));

	return ALP_CLOCK;
}

u32 si_ilp_clock(si_t *sih)
{
	if (PMUCTL_ENAB(sih))
		return si_pmu_ilp_clock(sih, si_osh(sih));

	return ILP_CLOCK;
}

/* set chip watchdog reset timer to fire in 'ticks' */
#ifdef BRCM_FULLMAC
void
si_watchdog(si_t *sih, uint ticks)
{
	if (PMUCTL_ENAB(sih)) {

		if ((sih->chip == BCM4319_CHIP_ID) && (sih->chiprev == 0) &&
			(ticks != 0)) {
			si_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t,
			clk_ctl_st), ~0, 0x2);
			si_setcore(sih, USB20D_CORE_ID, 0);
			si_core_disable(sih, 1);
			si_setcore(sih, CC_CORE_ID, 0);
		}

		if (ticks == 1)
			ticks = 2;
		si_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t, pmuwatchdog),
			~0, ticks);
	} else {
		/* instant NMI */
		si_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t, watchdog),
			~0, ticks);
	}
}
#else
void si_watchdog(si_t *sih, uint ticks)
{
	uint nb, maxt;

	if (PMUCTL_ENAB(sih)) {

		if ((CHIPID(sih->chip) == BCM4319_CHIP_ID) &&
		    (CHIPREV(sih->chiprev) == 0) && (ticks != 0)) {
			si_corereg(sih, SI_CC_IDX,
				   offsetof(chipcregs_t, clk_ctl_st), ~0, 0x2);
			si_setcore(sih, USB20D_CORE_ID, 0);
			si_core_disable(sih, 1);
			si_setcore(sih, CC_CORE_ID, 0);
		}

		nb = (sih->ccrev < 26) ? 16 : ((sih->ccrev >= 37) ? 32 : 24);
		/* The mips compiler uses the sllv instruction,
		 * so we specially handle the 32-bit case.
		 */
		if (nb == 32)
			maxt = 0xffffffff;
		else
			maxt = ((1 << nb) - 1);

		if (ticks == 1)
			ticks = 2;
		else if (ticks > maxt)
			ticks = maxt;

		si_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t, pmuwatchdog),
			   ~0, ticks);
	} else {
		/* make sure we come up in fast clock mode; or if clearing, clear clock */
		si_clkctl_cc(sih, ticks ? CLK_FAST : CLK_DYNAMIC);
		maxt = (1 << 28) - 1;
		if (ticks > maxt)
			ticks = maxt;

		si_corereg(sih, SI_CC_IDX, offsetof(chipcregs_t, watchdog), ~0,
			   ticks);
	}
}
#endif

/* return the slow clock source - LPO, XTAL, or PCI */
static uint si_slowclk_src(si_info_t *sii)
{
	chipcregs_t *cc;

	ASSERT(SI_FAST(sii) || si_coreid(&sii->pub) == CC_CORE_ID);

	if (sii->pub.ccrev < 6) {
		if ((BUSTYPE(sii->pub.bustype) == PCI_BUS) &&
		    (OSL_PCI_READ_CONFIG(sii->osh, PCI_GPIO_OUT, sizeof(u32))
		     & PCI_CFG_GPIO_SCS))
			return SCC_SS_PCI;
		else
			return SCC_SS_XTAL;
	} else if (sii->pub.ccrev < 10) {
		cc = (chipcregs_t *) si_setcoreidx(&sii->pub, sii->curidx);
		return R_REG(sii->osh, &cc->slow_clk_ctl) & SCC_SS_MASK;
	} else			/* Insta-clock */
		return SCC_SS_XTAL;
}

/* return the ILP (slowclock) min or max frequency */
static uint si_slowclk_freq(si_info_t *sii, bool max_freq, chipcregs_t *cc)
{
	u32 slowclk;
	uint div;

	ASSERT(SI_FAST(sii) || si_coreid(&sii->pub) == CC_CORE_ID);

	/* shouldn't be here unless we've established the chip has dynamic clk control */
	ASSERT(R_REG(sii->osh, &cc->capabilities) & CC_CAP_PWR_CTL);

	slowclk = si_slowclk_src(sii);
	if (sii->pub.ccrev < 6) {
		if (slowclk == SCC_SS_PCI)
			return max_freq ? (PCIMAXFREQ / 64)
				: (PCIMINFREQ / 64);
		else
			return max_freq ? (XTALMAXFREQ / 32)
				: (XTALMINFREQ / 32);
	} else if (sii->pub.ccrev < 10) {
		div = 4 *
		    (((R_REG(sii->osh, &cc->slow_clk_ctl) & SCC_CD_MASK) >>
		      SCC_CD_SHIFT) + 1);
		if (slowclk == SCC_SS_LPO)
			return max_freq ? LPOMAXFREQ : LPOMINFREQ;
		else if (slowclk == SCC_SS_XTAL)
			return max_freq ? (XTALMAXFREQ / div)
				: (XTALMINFREQ / div);
		else if (slowclk == SCC_SS_PCI)
			return max_freq ? (PCIMAXFREQ / div)
				: (PCIMINFREQ / div);
		else
			ASSERT(0);
	} else {
		/* Chipc rev 10 is InstaClock */
		div = R_REG(sii->osh, &cc->system_clk_ctl) >> SYCC_CD_SHIFT;
		div = 4 * (div + 1);
		return max_freq ? XTALMAXFREQ : (XTALMINFREQ / div);
	}
	return 0;
}

static void si_clkctl_setdelay(si_info_t *sii, void *chipcregs)
{
	chipcregs_t *cc = (chipcregs_t *) chipcregs;
	uint slowmaxfreq, pll_delay, slowclk;
	uint pll_on_delay, fref_sel_delay;

	pll_delay = PLL_DELAY;

	/* If the slow clock is not sourced by the xtal then add the xtal_on_delay
	 * since the xtal will also be powered down by dynamic clk control logic.
	 */

	slowclk = si_slowclk_src(sii);
	if (slowclk != SCC_SS_XTAL)
		pll_delay += XTAL_ON_DELAY;

	/* Starting with 4318 it is ILP that is used for the delays */
	slowmaxfreq =
	    si_slowclk_freq(sii, (sii->pub.ccrev >= 10) ? false : true, cc);

	pll_on_delay = ((slowmaxfreq * pll_delay) + 999999) / 1000000;
	fref_sel_delay = ((slowmaxfreq * FREF_DELAY) + 999999) / 1000000;

	W_REG(sii->osh, &cc->pll_on_delay, pll_on_delay);
	W_REG(sii->osh, &cc->fref_sel_delay, fref_sel_delay);
}

/* initialize power control delay registers */
void si_clkctl_init(si_t *sih)
{
	si_info_t *sii;
	uint origidx = 0;
	chipcregs_t *cc;
	bool fast;

	if (!CCCTL_ENAB(sih))
		return;

	sii = SI_INFO(sih);
	fast = SI_FAST(sii);
	if (!fast) {
		origidx = sii->curidx;
		cc = (chipcregs_t *) si_setcore(sih, CC_CORE_ID, 0);
		if (cc == NULL)
			return;
	} else {
		cc = (chipcregs_t *) CCREGS_FAST(sii);
		if (cc == NULL)
			return;
	}
	ASSERT(cc != NULL);

	/* set all Instaclk chip ILP to 1 MHz */
	if (sih->ccrev >= 10)
		SET_REG(sii->osh, &cc->system_clk_ctl, SYCC_CD_MASK,
			(ILP_DIV_1MHZ << SYCC_CD_SHIFT));

	si_clkctl_setdelay(sii, (void *)cc);

	if (!fast)
		si_setcoreidx(sih, origidx);
}

/* return the value suitable for writing to the dot11 core FAST_PWRUP_DELAY register */
u16 si_clkctl_fast_pwrup_delay(si_t *sih)
{
	si_info_t *sii;
	uint origidx = 0;
	chipcregs_t *cc;
	uint slowminfreq;
	u16 fpdelay;
	uint intr_val = 0;
	bool fast;

	sii = SI_INFO(sih);
	if (PMUCTL_ENAB(sih)) {
		INTR_OFF(sii, intr_val);
		fpdelay = si_pmu_fast_pwrup_delay(sih, sii->osh);
		INTR_RESTORE(sii, intr_val);
		return fpdelay;
	}

	if (!CCCTL_ENAB(sih))
		return 0;

	fast = SI_FAST(sii);
	fpdelay = 0;
	if (!fast) {
		origidx = sii->curidx;
		INTR_OFF(sii, intr_val);
		cc = (chipcregs_t *) si_setcore(sih, CC_CORE_ID, 0);
		if (cc == NULL)
			goto done;
	} else {
		cc = (chipcregs_t *) CCREGS_FAST(sii);
		if (cc == NULL)
			goto done;
	}
	ASSERT(cc != NULL);

	slowminfreq = si_slowclk_freq(sii, false, cc);
	fpdelay = (((R_REG(sii->osh, &cc->pll_on_delay) + 2) * 1000000) +
		   (slowminfreq - 1)) / slowminfreq;

 done:
	if (!fast) {
		si_setcoreidx(sih, origidx);
		INTR_RESTORE(sii, intr_val);
	}
	return fpdelay;
}

/* turn primary xtal and/or pll off/on */
int si_clkctl_xtal(si_t *sih, uint what, bool on)
{
	si_info_t *sii;
	u32 in, out, outen;

	sii = SI_INFO(sih);

	switch (BUSTYPE(sih->bustype)) {

#ifdef BCMSDIO
	case SDIO_BUS:
		return -1;
#endif				/* BCMSDIO */

	case PCI_BUS:
		/* pcie core doesn't have any mapping to control the xtal pu */
		if (PCIE(sii))
			return -1;

		in = OSL_PCI_READ_CONFIG(sii->osh, PCI_GPIO_IN, sizeof(u32));
		out =
		    OSL_PCI_READ_CONFIG(sii->osh, PCI_GPIO_OUT, sizeof(u32));
		outen =
		    OSL_PCI_READ_CONFIG(sii->osh, PCI_GPIO_OUTEN,
					sizeof(u32));

		/*
		 * Avoid glitching the clock if GPRS is already using it.
		 * We can't actually read the state of the PLLPD so we infer it
		 * by the value of XTAL_PU which *is* readable via gpioin.
		 */
		if (on && (in & PCI_CFG_GPIO_XTAL))
			return 0;

		if (what & XTAL)
			outen |= PCI_CFG_GPIO_XTAL;
		if (what & PLL)
			outen |= PCI_CFG_GPIO_PLL;

		if (on) {
			/* turn primary xtal on */
			if (what & XTAL) {
				out |= PCI_CFG_GPIO_XTAL;
				if (what & PLL)
					out |= PCI_CFG_GPIO_PLL;
				OSL_PCI_WRITE_CONFIG(sii->osh, PCI_GPIO_OUT,
						     sizeof(u32), out);
				OSL_PCI_WRITE_CONFIG(sii->osh, PCI_GPIO_OUTEN,
						     sizeof(u32), outen);
				udelay(XTAL_ON_DELAY);
			}

			/* turn pll on */
			if (what & PLL) {
				out &= ~PCI_CFG_GPIO_PLL;
				OSL_PCI_WRITE_CONFIG(sii->osh, PCI_GPIO_OUT,
						     sizeof(u32), out);
				mdelay(2);
			}
		} else {
			if (what & XTAL)
				out &= ~PCI_CFG_GPIO_XTAL;
			if (what & PLL)
				out |= PCI_CFG_GPIO_PLL;
			OSL_PCI_WRITE_CONFIG(sii->osh, PCI_GPIO_OUT,
					     sizeof(u32), out);
			OSL_PCI_WRITE_CONFIG(sii->osh, PCI_GPIO_OUTEN,
					     sizeof(u32), outen);
		}

	default:
		return -1;
	}

	return 0;
}

/*
 *  clock control policy function throught chipcommon
 *
 *    set dynamic clk control mode (forceslow, forcefast, dynamic)
 *    returns true if we are forcing fast clock
 *    this is a wrapper over the next internal function
 *      to allow flexible policy settings for outside caller
 */
bool si_clkctl_cc(si_t *sih, uint mode)
{
	si_info_t *sii;

	sii = SI_INFO(sih);

	/* chipcommon cores prior to rev6 don't support dynamic clock control */
	if (sih->ccrev < 6)
		return false;

	if (PCI_FORCEHT(sii))
		return mode == CLK_FAST;

	return _si_clkctl_cc(sii, mode);
}

/* clk control mechanism through chipcommon, no policy checking */
static bool _si_clkctl_cc(si_info_t *sii, uint mode)
{
	uint origidx = 0;
	chipcregs_t *cc;
	u32 scc;
	uint intr_val = 0;
	bool fast = SI_FAST(sii);

	/* chipcommon cores prior to rev6 don't support dynamic clock control */
	if (sii->pub.ccrev < 6)
		return false;

	/* Chips with ccrev 10 are EOL and they don't have SYCC_HR which we use below */
	ASSERT(sii->pub.ccrev != 10);

	if (!fast) {
		INTR_OFF(sii, intr_val);
		origidx = sii->curidx;

		if ((BUSTYPE(sii->pub.bustype) == SI_BUS) &&
		    si_setcore(&sii->pub, MIPS33_CORE_ID, 0) &&
		    (si_corerev(&sii->pub) <= 7) && (sii->pub.ccrev >= 10))
			goto done;

		cc = (chipcregs_t *) si_setcore(&sii->pub, CC_CORE_ID, 0);
	} else {
		cc = (chipcregs_t *) CCREGS_FAST(sii);
		if (cc == NULL)
			goto done;
	}
	ASSERT(cc != NULL);

	if (!CCCTL_ENAB(&sii->pub) && (sii->pub.ccrev < 20))
		goto done;

	switch (mode) {
	case CLK_FAST:		/* FORCEHT, fast (pll) clock */
		if (sii->pub.ccrev < 10) {
			/* don't forget to force xtal back on before we clear SCC_DYN_XTAL.. */
			si_clkctl_xtal(&sii->pub, XTAL, ON);
			SET_REG(sii->osh, &cc->slow_clk_ctl,
				(SCC_XC | SCC_FS | SCC_IP), SCC_IP);
		} else if (sii->pub.ccrev < 20) {
			OR_REG(sii->osh, &cc->system_clk_ctl, SYCC_HR);
		} else {
			OR_REG(sii->osh, &cc->clk_ctl_st, CCS_FORCEHT);
		}

		/* wait for the PLL */
		if (PMUCTL_ENAB(&sii->pub)) {
			u32 htavail = CCS_HTAVAIL;
			SPINWAIT(((R_REG(sii->osh, &cc->clk_ctl_st) & htavail)
				  == 0), PMU_MAX_TRANSITION_DLY);
			ASSERT(R_REG(sii->osh, &cc->clk_ctl_st) & htavail);
		} else {
			udelay(PLL_DELAY);
		}
		break;

	case CLK_DYNAMIC:	/* enable dynamic clock control */
		if (sii->pub.ccrev < 10) {
			scc = R_REG(sii->osh, &cc->slow_clk_ctl);
			scc &= ~(SCC_FS | SCC_IP | SCC_XC);
			if ((scc & SCC_SS_MASK) != SCC_SS_XTAL)
				scc |= SCC_XC;
			W_REG(sii->osh, &cc->slow_clk_ctl, scc);

			/* for dynamic control, we have to release our xtal_pu "force on" */
			if (scc & SCC_XC)
				si_clkctl_xtal(&sii->pub, XTAL, OFF);
		} else if (sii->pub.ccrev < 20) {
			/* Instaclock */
			AND_REG(sii->osh, &cc->system_clk_ctl, ~SYCC_HR);
		} else {
			AND_REG(sii->osh, &cc->clk_ctl_st, ~CCS_FORCEHT);
		}
		break;

	default:
		ASSERT(0);
	}

 done:
	if (!fast) {
		si_setcoreidx(&sii->pub, origidx);
		INTR_RESTORE(sii, intr_val);
	}
	return mode == CLK_FAST;
}

/* Build device path. Support SI, PCI, and JTAG for now. */
int si_devpath(si_t *sih, char *path, int size)
{
	int slen;

	ASSERT(path != NULL);
	ASSERT(size >= SI_DEVPATH_BUFSZ);

	if (!path || size <= 0)
		return -1;

	switch (BUSTYPE(sih->bustype)) {
	case SI_BUS:
	case JTAG_BUS:
		slen = snprintf(path, (size_t) size, "sb/%u/", si_coreidx(sih));
		break;
	case PCI_BUS:
		ASSERT((SI_INFO(sih))->osh != NULL);
		slen = snprintf(path, (size_t) size, "pci/%u/%u/",
				OSL_PCI_BUS((SI_INFO(sih))->osh),
				OSL_PCI_SLOT((SI_INFO(sih))->osh));
		break;

#ifdef BCMSDIO
	case SDIO_BUS:
		SI_ERROR(("si_devpath: device 0 assumed\n"));
		slen = snprintf(path, (size_t) size, "sd/%u/", si_coreidx(sih));
		break;
#endif
	default:
		slen = -1;
		ASSERT(0);
		break;
	}

	if (slen < 0 || slen >= size) {
		path[0] = '\0';
		return -1;
	}

	return 0;
}

/* Get a variable, but only if it has a devpath prefix */
char *si_getdevpathvar(si_t *sih, const char *name)
{
	char varname[SI_DEVPATH_BUFSZ + 32];

	si_devpathvar(sih, varname, sizeof(varname), name);

	return getvar(NULL, varname);
}

/* Get a variable, but only if it has a devpath prefix */
int si_getdevpathintvar(si_t *sih, const char *name)
{
#if defined(BCMBUSTYPE) && (BCMBUSTYPE == SI_BUS)
	return getintvar(NULL, name);
#else
	char varname[SI_DEVPATH_BUFSZ + 32];

	si_devpathvar(sih, varname, sizeof(varname), name);

	return getintvar(NULL, varname);
#endif
}

char *si_getnvramflvar(si_t *sih, const char *name)
{
	return getvar(NULL, name);
}

/* Concatenate the dev path with a varname into the given 'var' buffer
 * and return the 'var' pointer.
 * Nothing is done to the arguments if len == 0 or var is NULL, var is still returned.
 * On overflow, the first char will be set to '\0'.
 */
static char *si_devpathvar(si_t *sih, char *var, int len, const char *name)
{
	uint path_len;

	if (!var || len <= 0)
		return var;

	if (si_devpath(sih, var, len) == 0) {
		path_len = strlen(var);

		if (strlen(name) + 1 > (uint) (len - path_len))
			var[0] = '\0';
		else
			strncpy(var + path_len, name, len - path_len - 1);
	}

	return var;
}

/* return true if PCIE capability exists in the pci config space */
static __used bool si_ispcie(si_info_t *sii)
{
	u8 cap_ptr;

	if (BUSTYPE(sii->pub.bustype) != PCI_BUS)
		return false;

	cap_ptr =
	    pcicore_find_pci_capability(sii->osh, PCI_CAP_PCIECAP_ID, NULL,
					NULL);
	if (!cap_ptr)
		return false;

	return true;
}

#ifdef BCMSDIO
/* initialize the sdio core */
void si_sdio_init(si_t *sih)
{
	si_info_t *sii = SI_INFO(sih);

	if (((sih->buscoretype == PCMCIA_CORE_ID) && (sih->buscorerev >= 8)) ||
	    (sih->buscoretype == SDIOD_CORE_ID)) {
		uint idx;
		sdpcmd_regs_t *sdpregs;

		/* get the current core index */
		idx = sii->curidx;
		ASSERT(idx == si_findcoreidx(sih, D11_CORE_ID, 0));

		/* switch to sdio core */
		sdpregs = (sdpcmd_regs_t *) si_setcore(sih, PCMCIA_CORE_ID, 0);
		if (!sdpregs)
			sdpregs =
			    (sdpcmd_regs_t *) si_setcore(sih, SDIOD_CORE_ID, 0);
		ASSERT(sdpregs);

		SI_MSG(("si_sdio_init: For PCMCIA/SDIO Corerev %d, enable ints from core %d " "through SD core %d (%p)\n", sih->buscorerev, idx, sii->curidx, sdpregs));

		/* enable backplane error and core interrupts */
		W_REG(sii->osh, &sdpregs->hostintmask, I_SBINT);
		W_REG(sii->osh, &sdpregs->sbintmask,
		      (I_SB_SERR | I_SB_RESPERR | (1 << idx)));

		/* switch back to previous core */
		si_setcoreidx(sih, idx);
	}

	/* enable interrupts */
	bcmsdh_intr_enable(sii->sdh);

}
#endif				/* BCMSDIO */

bool si_pci_war16165(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);

	return PCI(sii) && (sih->buscorerev <= 10);
}

void si_pci_up(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);

	/* if not pci bus, we're done */
	if (BUSTYPE(sih->bustype) != PCI_BUS)
		return;

	if (PCI_FORCEHT(sii))
		_si_clkctl_cc(sii, CLK_FAST);

	if (PCIE(sii))
		pcicore_up(sii->pch, SI_PCIUP);

}

/* Unconfigure and/or apply various WARs when system is going to sleep mode */
void si_pci_sleep(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);

	pcicore_sleep(sii->pch);
}

/* Unconfigure and/or apply various WARs when going down */
void si_pci_down(si_t *sih)
{
	si_info_t *sii;

	sii = SI_INFO(sih);

	/* if not pci bus, we're done */
	if (BUSTYPE(sih->bustype) != PCI_BUS)
		return;

	/* release FORCEHT since chip is going to "down" state */
	if (PCI_FORCEHT(sii))
		_si_clkctl_cc(sii, CLK_DYNAMIC);

	pcicore_down(sii->pch, SI_PCIDOWN);
}

/*
 * Configure the pci core for pci client (NIC) action
 * coremask is the bitvec of cores by index to be enabled.
 */
void si_pci_setup(si_t *sih, uint coremask)
{
	si_info_t *sii;
	struct sbpciregs *pciregs = NULL;
	u32 siflag = 0, w;
	uint idx = 0;

	sii = SI_INFO(sih);

	if (BUSTYPE(sii->pub.bustype) != PCI_BUS)
		return;

	ASSERT(PCI(sii) || PCIE(sii));
	ASSERT(sii->pub.buscoreidx != BADIDX);

	if (PCI(sii)) {
		/* get current core index */
		idx = sii->curidx;

		/* we interrupt on this backplane flag number */
		siflag = si_flag(sih);

		/* switch over to pci core */
		pciregs = (struct sbpciregs *)si_setcoreidx(sih, sii->pub.buscoreidx);
	}

	/*
	 * Enable sb->pci interrupts.  Assume
	 * PCI rev 2.3 support was added in pci core rev 6 and things changed..
	 */
	if (PCIE(sii) || (PCI(sii) && ((sii->pub.buscorerev) >= 6))) {
		/* pci config write to set this core bit in PCIIntMask */
		w = OSL_PCI_READ_CONFIG(sii->osh, PCI_INT_MASK, sizeof(u32));
		w |= (coremask << PCI_SBIM_SHIFT);
		OSL_PCI_WRITE_CONFIG(sii->osh, PCI_INT_MASK, sizeof(u32), w);
	} else {
		/* set sbintvec bit for our flag number */
		si_setint(sih, siflag);
	}

	if (PCI(sii)) {
		OR_REG(sii->osh, &pciregs->sbtopci2,
		       (SBTOPCI_PREF | SBTOPCI_BURST));
		if (sii->pub.buscorerev >= 11) {
			OR_REG(sii->osh, &pciregs->sbtopci2,
			       SBTOPCI_RC_READMULTI);
			w = R_REG(sii->osh, &pciregs->clkrun);
			W_REG(sii->osh, &pciregs->clkrun,
			      (w | PCI_CLKRUN_DSBL));
			w = R_REG(sii->osh, &pciregs->clkrun);
		}

		/* switch back to previous core */
		si_setcoreidx(sih, idx);
	}
}

/*
 * Fixup SROMless PCI device's configuration.
 * The current core may be changed upon return.
 */
int si_pci_fixcfg(si_t *sih)
{
	uint origidx, pciidx;
	struct sbpciregs *pciregs = NULL;
	sbpcieregs_t *pcieregs = NULL;
	void *regs = NULL;
	u16 val16, *reg16 = NULL;

	si_info_t *sii = SI_INFO(sih);

	ASSERT(BUSTYPE(sii->pub.bustype) == PCI_BUS);

	/* Fixup PI in SROM shadow area to enable the correct PCI core access */
	/* save the current index */
	origidx = si_coreidx(&sii->pub);

	/* check 'pi' is correct and fix it if not */
	if (sii->pub.buscoretype == PCIE_CORE_ID) {
		pcieregs =
		    (sbpcieregs_t *) si_setcore(&sii->pub, PCIE_CORE_ID, 0);
		regs = pcieregs;
		ASSERT(pcieregs != NULL);
		reg16 = &pcieregs->sprom[SRSH_PI_OFFSET];
	} else if (sii->pub.buscoretype == PCI_CORE_ID) {
		pciregs = (struct sbpciregs *)si_setcore(&sii->pub, PCI_CORE_ID, 0);
		regs = pciregs;
		ASSERT(pciregs != NULL);
		reg16 = &pciregs->sprom[SRSH_PI_OFFSET];
	}
	pciidx = si_coreidx(&sii->pub);
	val16 = R_REG(sii->osh, reg16);
	if (((val16 & SRSH_PI_MASK) >> SRSH_PI_SHIFT) != (u16) pciidx) {
		val16 =
		    (u16) (pciidx << SRSH_PI_SHIFT) | (val16 &
							  ~SRSH_PI_MASK);
		W_REG(sii->osh, reg16, val16);
	}

	/* restore the original index */
	si_setcoreidx(&sii->pub, origidx);

	pcicore_hwup(sii->pch);
	return 0;
}

/* mask&set gpiocontrol bits */
u32 si_gpiocontrol(si_t *sih, u32 mask, u32 val, u8 priority)
{
	uint regoff;

	regoff = 0;

	/* gpios could be shared on router platforms
	 * ignore reservation if it's high priority (e.g., test apps)
	 */
	if ((priority != GPIO_HI_PRIORITY) &&
	    (BUSTYPE(sih->bustype) == SI_BUS) && (val || mask)) {
		mask = priority ? (si_gpioreservation & mask) :
		    ((si_gpioreservation | mask) & ~(si_gpioreservation));
		val &= mask;
	}

	regoff = offsetof(chipcregs_t, gpiocontrol);
	return si_corereg(sih, SI_CC_IDX, regoff, mask, val);
}

/* Return the size of the specified SOCRAM bank */
static uint
socram_banksize(si_info_t *sii, sbsocramregs_t *regs, u8 index,
		u8 mem_type)
{
	uint banksize, bankinfo;
	uint bankidx = index | (mem_type << SOCRAM_BANKIDX_MEMTYPE_SHIFT);

	ASSERT(mem_type <= SOCRAM_MEMTYPE_DEVRAM);

	W_REG(sii->osh, &regs->bankidx, bankidx);
	bankinfo = R_REG(sii->osh, &regs->bankinfo);
	banksize =
	    SOCRAM_BANKINFO_SZBASE * ((bankinfo & SOCRAM_BANKINFO_SZMASK) + 1);
	return banksize;
}

/* Return the RAM size of the SOCRAM core */
u32 si_socram_size(si_t *sih)
{
	si_info_t *sii;
	uint origidx;
	uint intr_val = 0;

	sbsocramregs_t *regs;
	bool wasup;
	uint corerev;
	u32 coreinfo;
	uint memsize = 0;

	sii = SI_INFO(sih);

	/* Block ints and save current core */
	INTR_OFF(sii, intr_val);
	origidx = si_coreidx(sih);

	/* Switch to SOCRAM core */
	regs = si_setcore(sih, SOCRAM_CORE_ID, 0);
	if (!regs)
		goto done;

	/* Get info for determining size */
	wasup = si_iscoreup(sih);
	if (!wasup)
		si_core_reset(sih, 0, 0);
	corerev = si_corerev(sih);
	coreinfo = R_REG(sii->osh, &regs->coreinfo);

	/* Calculate size from coreinfo based on rev */
	if (corerev == 0)
		memsize = 1 << (16 + (coreinfo & SRCI_MS0_MASK));
	else if (corerev < 3) {
		memsize = 1 << (SR_BSZ_BASE + (coreinfo & SRCI_SRBSZ_MASK));
		memsize *= (coreinfo & SRCI_SRNB_MASK) >> SRCI_SRNB_SHIFT;
	} else if ((corerev <= 7) || (corerev == 12)) {
		uint nb = (coreinfo & SRCI_SRNB_MASK) >> SRCI_SRNB_SHIFT;
		uint bsz = (coreinfo & SRCI_SRBSZ_MASK);
		uint lss = (coreinfo & SRCI_LSS_MASK) >> SRCI_LSS_SHIFT;
		if (lss != 0)
			nb--;
		memsize = nb * (1 << (bsz + SR_BSZ_BASE));
		if (lss != 0)
			memsize += (1 << ((lss - 1) + SR_BSZ_BASE));
	} else {
		u8 i;
		uint nb = (coreinfo & SRCI_SRNB_MASK) >> SRCI_SRNB_SHIFT;
		for (i = 0; i < nb; i++)
			memsize +=
			    socram_banksize(sii, regs, i, SOCRAM_MEMTYPE_RAM);
	}

	/* Return to previous state and core */
	if (!wasup)
		si_core_disable(sih, 0);
	si_setcoreidx(sih, origidx);

 done:
	INTR_RESTORE(sii, intr_val);

	return memsize;
}

void si_chipcontrl_epa4331(si_t *sih, bool on)
{
	si_info_t *sii;
	chipcregs_t *cc;
	uint origidx;
	u32 val;

	sii = SI_INFO(sih);
	origidx = si_coreidx(sih);

	cc = (chipcregs_t *) si_setcore(sih, CC_CORE_ID, 0);

	val = R_REG(sii->osh, &cc->chipcontrol);

	if (on) {
		if (sih->chippkg == 9 || sih->chippkg == 0xb) {
			/* Ext PA Controls for 4331 12x9 Package */
			W_REG(sii->osh, &cc->chipcontrol, val |
			      (CCTRL4331_EXTPA_EN |
			       CCTRL4331_EXTPA_ON_GPIO2_5));
		} else {
			/* Ext PA Controls for 4331 12x12 Package */
			W_REG(sii->osh, &cc->chipcontrol,
			      val | (CCTRL4331_EXTPA_EN));
		}
	} else {
		val &= ~(CCTRL4331_EXTPA_EN | CCTRL4331_EXTPA_ON_GPIO2_5);
		W_REG(sii->osh, &cc->chipcontrol, val);
	}

	si_setcoreidx(sih, origidx);
}

/* Enable BT-COEX & Ex-PA for 4313 */
void si_epa_4313war(si_t *sih)
{
	si_info_t *sii;
	chipcregs_t *cc;
	uint origidx;

	sii = SI_INFO(sih);
	origidx = si_coreidx(sih);

	cc = (chipcregs_t *) si_setcore(sih, CC_CORE_ID, 0);

	/* EPA Fix */
	W_REG(sii->osh, &cc->gpiocontrol,
	      R_REG(sii->osh, &cc->gpiocontrol) | GPIO_CTRL_EPA_EN_MASK);

	si_setcoreidx(sih, origidx);
}

/* check if the device is removed */
bool si_deviceremoved(si_t *sih)
{
	u32 w;
	si_info_t *sii;

	sii = SI_INFO(sih);

	switch (BUSTYPE(sih->bustype)) {
	case PCI_BUS:
		ASSERT(sii->osh != NULL);
		w = OSL_PCI_READ_CONFIG(sii->osh, PCI_CFG_VID, sizeof(u32));
		if ((w & 0xFFFF) != VENDOR_BROADCOM)
			return true;
		break;
	}
	return false;
}

bool si_is_sprom_available(si_t *sih)
{
	if (sih->ccrev >= 31) {
		si_info_t *sii;
		uint origidx;
		chipcregs_t *cc;
		u32 sromctrl;

		if ((sih->cccaps & CC_CAP_SROM) == 0)
			return false;

		sii = SI_INFO(sih);
		origidx = sii->curidx;
		cc = si_setcoreidx(sih, SI_CC_IDX);
		sromctrl = R_REG(sii->osh, &cc->sromcontrol);
		si_setcoreidx(sih, origidx);
		return sromctrl & SRC_PRESENT;
	}

	switch (CHIPID(sih->chip)) {
	case BCM4329_CHIP_ID:
		return (sih->chipst & CST4329_SPROM_SEL) != 0;
	case BCM4319_CHIP_ID:
		return (sih->chipst & CST4319_SPROM_SEL) != 0;
	case BCM4336_CHIP_ID:
		return (sih->chipst & CST4336_SPROM_PRESENT) != 0;
	case BCM4330_CHIP_ID:
		return (sih->chipst & CST4330_SPROM_PRESENT) != 0;
	case BCM4313_CHIP_ID:
		return (sih->chipst & CST4313_SPROM_PRESENT) != 0;
	case BCM4331_CHIP_ID:
		return (sih->chipst & CST4331_SPROM_PRESENT) != 0;
	default:
		return true;
	}
}

bool si_is_otp_disabled(si_t *sih)
{
	switch (CHIPID(sih->chip)) {
	case BCM4329_CHIP_ID:
		return (sih->chipst & CST4329_SPROM_OTP_SEL_MASK) ==
		    CST4329_OTP_PWRDN;
	case BCM4319_CHIP_ID:
		return (sih->chipst & CST4319_SPROM_OTP_SEL_MASK) ==
		    CST4319_OTP_PWRDN;
	case BCM4336_CHIP_ID:
		return (sih->chipst & CST4336_OTP_PRESENT) == 0;
	case BCM4330_CHIP_ID:
		return (sih->chipst & CST4330_OTP_PRESENT) == 0;
	case BCM4313_CHIP_ID:
		return (sih->chipst & CST4313_OTP_PRESENT) == 0;
		/* These chips always have their OTP on */
	case BCM43224_CHIP_ID:
	case BCM43225_CHIP_ID:
	case BCM43421_CHIP_ID:
	case BCM43235_CHIP_ID:
	case BCM43236_CHIP_ID:
	case BCM43238_CHIP_ID:
	case BCM4331_CHIP_ID:
	default:
		return false;
	}
}

bool si_is_otp_powered(si_t *sih)
{
	if (PMUCTL_ENAB(sih))
		return si_pmu_is_otp_powered(sih, si_osh(sih));
	return true;
}

void si_otp_power(si_t *sih, bool on)
{
	if (PMUCTL_ENAB(sih))
		si_pmu_otp_power(sih, si_osh(sih), on);
	udelay(1000);
}

