/*
 *  include/linux/regulator/rt5025-regulator.h
 *  Include header file to Richtek RT5025 Regulator driver
 *
 *  Copyright (C) 2013 Richtek Technology Corp.
 *  cy_huang <cy_huang@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef __LINUX_RT5025_REGULATOR_H
#define __LINUX_RT5025_REGULATOR_H

#define RT5025_DCDCVOUT1	RT5025_REG_DCDCCTL1
#define RT5025_DCDCVOUT2	RT5025_REG_DCDCCTL2
#define RT5025_DCDCVOUT3	RT5025_REG_DCDCCTL3
#define RT5025_DCDCVOUT4	RT5025_REG_VRCCTL
#define RT5025_LDOVOUT1	RT5025_REG_LDOCTL1
#define RT5025_LDOVOUT2	RT5025_REG_LDOCTL2
#define RT5025_LDOVOUT3	RT5025_REG_LDOCTL3
#define RT5025_LDOVOUT4	RT5025_REG_LDOCTL4
#define RT5025_LDOVOUT5	RT5025_REG_LDOCTL5
#define RT5025_LDOVOUT6	RT5025_REG_LDOCTL6

#define RT5025_DCDCVOUT_SHIFT1	2
#define RT5025_DCDCVOUT_SHIFT2	1
#define RT5025_DCDCVOUT_SHIFT3	2
#define RT5025_DCDCVOUT_SHIFT4	0
#define RT5025_LDOVOUT_SHIFT1	0
#define RT5025_LDOVOUT_SHIFT2	0
#define RT5025_LDOVOUT_SHIFT3	3
#define RT5025_LDOVOUT_SHIFT4	3
#define RT5025_LDOVOUT_SHIFT5	3
#define RT5025_LDOVOUT_SHIFT6	3

#define RT5025_DCDCVOUT_MASK1	0xFC
#define RT5025_DCDCVOUT_MASK2	0xFE
#define RT5025_DCDCVOUT_MASK3	0xFC
#define RT5025_DCDCVOUT_MASK4	0x0F
#define RT5025_LDOVOUT_MASK1	0x7F
#define RT5025_LDOVOUT_MASK2	0x7F
#define RT5025_LDOVOUT_MASK3	0xF8
#define RT5025_LDOVOUT_MASK4	0xF8
#define RT5025_LDOVOUT_MASK5	0xF8
#define RT5025_LDOVOUT_MASK6	0xF8

#define RT5025_DCDCEN_MASK1	0x01
#define RT5025_DCDCEN_MASK2	0x02
#define RT5025_DCDCEN_MASK3	0x04
#define RT5025_DCDCEN_MASK4	0x08
#define RT5025_LDOEN_MASK1	0x01
#define RT5025_LDOEN_MASK2	0x02
#define RT5025_LDOEN_MASK3	0x04
#define RT5025_LDOEN_MASK4	0x08
#define RT5025_LDOEN_MASK5	0x10
#define RT5025_LDOEN_MASK6	0x20

#define RT5025_DCDCMODE_REG1	RT5025_REG_VRCCTL
#define RT5025_DCDCMODE_REG2	RT5025_REG_VRCCTL
#define RT5025_DCDCMODE_REG3	RT5025_REG_VRCCTL
#define RT5025_DCDCMODE_REG4	0

#define RT5025_DCDCMODE_MASK1	0x40
#define RT5025_DCDCMODE_MASK2	0x20
#define RT5025_DCDCMODE_MASK3	0x10
#define RT5025_DCDCMODE_MASK4	0x00

#define RT5025_DCDCRAMP_MASK1	0x03
#define RT5025_DCDCRAMP_MASK2	0x01
#define RT5025_DCDCRAMP_MASK3	0x03
#define RT5025_DCDCRAMP_MASK4	0x00

#endif  /* __LINUX_RT5025_REGULATOR_H */
