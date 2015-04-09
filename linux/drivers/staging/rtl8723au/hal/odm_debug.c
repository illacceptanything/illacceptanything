/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#include "odm_precomp.h"

void ODM_InitDebugSetting23a(struct dm_odm_t *pDM_Odm)
{
	pDM_Odm->DebugLevel = ODM_DBG_TRACE;
	pDM_Odm->DebugComponents = 0;
}

u32 GlobalDebugLevel23A;
