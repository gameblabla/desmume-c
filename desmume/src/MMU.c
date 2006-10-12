/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#define RENDER3D

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "debug.h"
#include "NDSSystem.h"
#include "cflash.h"

#include "nds/interrupts.h"
#include "nds/video.h"
#include "nds/system.h"
#include "nds/serial.h"
#include "nds/card.h"

#define ROM_MASK 3

//#define LOG_CARD
//#define LOG_GPU
//#define LOG_DMA
//#define LOG_DMA2
//#define LOG_DIV

extern BOOL execute;
extern BOOL click;
extern NDSSystem nds;

char szRomPath[512];

MMU_struct MMU;

u8 * MMU_ARM9_MEM_MAP[256]={
	//0X
	ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, 
	//1X
	//ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, ARM9Mem.ARM9_ITCM, 
	ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, ARM9Mem.ARM9_WRAM, 
	//2X
	ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM,
	//3x
	MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, 
	//4X
	ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, ARM9Mem.ARM9_REG, 
	//5X
	ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, ARM9Mem.ARM9_VMEM, 
	//60 61
	ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, 
	//62 63
	ARM9Mem.ARM9_BBG, ARM9Mem.ARM9_BBG,
	//64 65
	ARM9Mem.ARM9_AOBJ, ARM9Mem.ARM9_AOBJ,
	//66 67
	ARM9Mem.ARM9_BOBJ, ARM9Mem.ARM9_BOBJ,
	//68 69 ..
	ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD, ARM9Mem.ARM9_LCD,
	//7X
	ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM, ARM9Mem.ARM9_OAM,
	//8X
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	//9X
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	//AX
	MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, 
	//BX
	//CX
	//DX
	//EX
	//FX
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS, ARM9Mem.ARM9_BIOS,
};
	   
u32 MMU_ARM9_MEM_MASK[256]={
	//0X
	0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
	//1X
	0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
	//0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
	//2x
	0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF,
	//3X
	0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
	//4X
	0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
	//5X
	0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 
	//60 61
	0x7FFFF, 0x7FFFF, 
	//62 63 
	0x1FFFF, 0x1FFFF,
	//64 65
	0x3FFFF, 0x3FFFF,
	//66 67
	0x1FFFF, 0x1FFFF,
	//68 69 ....
	0xFFFFF, 0xFFFFF, 0xFFFFF, 0xFFFFF, 0xFFFFF, 0xFFFFF, 0xFFFFF, 0xFFFFF, 
	//7X		 
	0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 0x7FF, 
	//8X
	ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK,
	//9x
	ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK,
	//AX
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	//BX
	//CX
	//DX
	//EX
	//FX
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x0003, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
};

u8 * MMU_ARM7_MEM_MAP[256]={
	//0X
	MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, MMU.ARM7_BIOS, 
	//1X
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	//2X
	ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, ARM9Mem.MAIN_MEM, 
	//30 - 37
	MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, MMU.SWIRAM, 
	//38 - 3F
	MMU.ARM7_ERAM, MMU.ARM7_ERAM, MMU.ARM7_ERAM, MMU.ARM7_ERAM, MMU.ARM7_ERAM, MMU.ARM7_ERAM, MMU.ARM7_ERAM, MMU.ARM7_ERAM,
	//40 - 47
	MMU.ARM7_REG, MMU.ARM7_REG, MMU.ARM7_REG, MMU.ARM7_REG, MMU.ARM7_REG, MMU.ARM7_REG, MMU.ARM7_REG, MMU.ARM7_REG,
	//48 - 4F
	MMU.ARM7_WIRAM, MMU.ARM7_WIRAM, MMU.ARM7_WIRAM, MMU.ARM7_WIRAM, MMU.ARM7_WIRAM, MMU.ARM7_WIRAM, MMU.ARM7_WIRAM, MMU.ARM7_WIRAM,
	//5X
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	//6X
	ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG, ARM9Mem.ARM9_ABG,
	//7X
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	//8X
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	//9X
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	//AX
	MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, MMU.CART_RAM, 
	//BX
	//CX
	//DX
	//EX
	//FX
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, 
	MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM, MMU.UNUSED_RAM,
};

u32 MMU_ARM7_MEM_MASK[256]={
	//0X
	0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 
	//1X
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	//2x
	0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF, 0x3FFFFF,
	//30 - 37
	0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 
	//38 - 3F
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
	//40 - 47
	0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 
	//48 - 4F
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 
	//5X
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	//6X
	0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF, 0x3FFFF,
	//7X
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	//8X
	ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK,
	//9x
	ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK, ROM_MASK,
	//AX
	0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	//BX
	//CX
	//DX
	//EX
	//FX
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
	0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003, 0x0003,
};

u32 MMU_ARM9_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

u32 MMU_ARM9_WAIT32[16]={
	1, 1, 1, 1, 1, 2, 2, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

u32 MMU_ARM7_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

u32 MMU_ARM7_WAIT32[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

void MMUInit(void) {
	int i;

	LOG("MMU init\n");

	memset(&MMU, 0, sizeof(MMU_struct));

	MMU.CART_ROM = MMU.UNUSED_RAM;

        for(i = 0x80; i<0xA0; ++i)
        {
           MMU_ARM9_MEM_MAP[i] = MMU.CART_ROM;
           MMU_ARM7_MEM_MAP[i] = MMU.CART_ROM;
        }

	MMU.MMU_MEM[0] = MMU_ARM9_MEM_MAP;
	MMU.MMU_MEM[1] = MMU_ARM7_MEM_MAP;
	MMU.MMU_MASK[0]= MMU_ARM9_MEM_MASK;
	MMU.MMU_MASK[1] = MMU_ARM7_MEM_MASK;

	MMU.ITCMRegion = 0x00800000;

	MMU.MMU_WAIT16[0] = MMU_ARM9_WAIT16;
	MMU.MMU_WAIT16[1] = MMU_ARM7_WAIT16;
	MMU.MMU_WAIT32[0] = MMU_ARM9_WAIT32;
	MMU.MMU_WAIT32[1] = MMU_ARM7_WAIT32;

	for(i = 0;i < 16;i++)
		FIFOInit(MMU.fifos + i);
	
        mc_init(&MMU.fw, MC_TYPE_FLASH);  /* init fw device */
        mc_alloc(&MMU.fw, NDS_FW_SIZE_V1);

        // Init Backup Memory device, this should really be done when the rom is loaded
        mc_init(&MMU.bupmem, MC_TYPE_EEPROM2);
        mc_alloc(&MMU.bupmem, 65536); // For now we're use 512Kbit support. Eventually this should be detected when rom is loaded
}

void MMUDeInit(void) {
	LOG("MMU deinit\n");
}

//Card rom & ram

u16 SPI_CNT = 0;
u16 SPI_CMD = 0;
u16 AUX_SPI_CNT = 0;
u16 AUX_SPI_CMD = 0;

u32 rom_mask = 0;

u32 DMASrc[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
u32 DMADst[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};


void MMU_setRom(u8 * rom, u32 mask)
{
    unsigned int i;
    MMU.CART_ROM = rom;
    
    for(i = 0x80; i<0xA0; ++i)
    {
	MMU_ARM9_MEM_MAP[i] = rom;
	MMU_ARM7_MEM_MAP[i] = rom;
	MMU_ARM9_MEM_MASK[i] = mask;
	MMU_ARM7_MEM_MASK[i] = mask;
    }
    rom_mask = mask;
}

void MMU_unsetRom()
{
    unsigned int i;
    MMU.CART_ROM=MMU.UNUSED_RAM;

    for(i = 0x80; i<0xA0; ++i)
    {
	MMU_ARM9_MEM_MAP[i] = MMU.UNUSED_RAM;
	MMU_ARM7_MEM_MAP[i] = MMU.UNUSED_RAM;
	MMU_ARM9_MEM_MASK[i] = ROM_MASK;
	MMU_ARM7_MEM_MASK[i] = ROM_MASK;
    }
    rom_mask = ROM_MASK;
}
char txt[80];	

u8 FASTCALL MMU_read8(u32 proc, u32 adr)
{
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==MMU.DTCMRegion))
	{
		return ARM9Mem.ARM9_DTCM[adr&0x3FFF];
	}
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
	   return (unsigned char)cflash_read(adr);

	adr &= 0x0FFFFFFF;
	switch(adr)
	{
		case 0x027FFCDC :
			return 0x20;
		case 0x027FFCDD :
			return 0x20;
		case 0x027FFCE2 :
		 //execute = FALSE;
			return 0xE0;
		case 0x027FFCE3 :
			return 0x80;
		/*case 0x04000300 :
			return 1;
		case 0x04000208 :
			execute = FALSE;*/
		default :
		    return MMU.MMU_MEM[proc][(adr>>20)&0xFF][adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF]];
	}
}



u16 FASTCALL MMU_read16(u32 proc, u32 adr)
{
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return ((u16 *)ARM9Mem.ARM9_DTCM)[(adr&0x3FFF)>>1];
	}
	
	// CFlash reading, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	   return (unsigned short)cflash_read(adr);
	
	adr &= 0x0FFFFFFF;
	
	if((adr>>24)==4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			case 0x04100000 :		/* TODO (clear): ??? */
				execute = FALSE;
				return 1;
				
			case REG_IME :
				return (u16)MMU.reg_IME[proc];
				
			case REG_IE :
				return (u16)MMU.reg_IE[proc];
			case REG_IE + 2 :
				return (u16)(MMU.reg_IE[proc]>>16);
				
			case REG_IF :
				return (u16)MMU.reg_IF[proc];
			case REG_IF + 2 :
				return (u16)(MMU.reg_IF[proc]>>16);
				
			case 0x04000100 :
			case 0x04000104 :
			case 0x04000108 :
			case 0x0400010C :
				return MMU.timer[proc][(adr&0xF)>>2];
			
			case 0x04000630 :
				LOG("vect res\r\n");	/* TODO (clear): ??? */
				//execute = FALSE;
				return 0;
			//case 0x27FFFAA :
			//case 0x27FFFAC :
			/*case 0x2330F84 :
				if(click) execute = FALSE;*/
			//case 0x27FF018 : execute = FALSE;
			case 0x04000300 :
				return 1;
			default :
				break;
		}
	}
	else
	{
		/* TODO (clear): i don't known what are these 'registers', perhaps reset vectors ... */
		switch(adr)
		{
			case 0x027FFCD8 :
				return (0x20<<4);
			case 0x027FFCDA :
				return (0x20<<4);
			case 0x027FFCDE : 
				return (0xE0<<4);
			case 0x027FFCE0 :
			//execute = FALSE;
				return (0x80<<4);
			default :
				break;
		}
	}
	
	/* Returns data from memory */
	return ((u16 *)(MMU.MMU_MEM[proc][(adr>>20)&0xFF]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>1];
}
	 
u32 FASTCALL MMU_read32(u32 proc, u32 adr)
{
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return ((u32 *)ARM9Mem.ARM9_DTCM)[(adr&0x3FFF)>>2];
	}
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
	   return (unsigned long)cflash_read(adr);
		
	adr &= 0x0FFFFFFF;

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			#ifdef RENDER3D
			case 0x04000600 :
				return ((OGLRender::nbpush[0]&1)<<13) | ((OGLRender::nbpush[2]&0x1F)<<8);
			#endif
			
			case REG_IME :
				return MMU.reg_IME[proc];
			case REG_IE :
				return MMU.reg_IE[proc];
			case REG_IF :
				return MMU.reg_IF[proc];
				
			case 0x04100000 :
			{
				u16 IPCFIFO_CNT = ((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1];
				if(IPCFIFO_CNT&0x8000)
				{
				//execute = FALSE;
				u32 fifonum = IPCFIFO+proc;
				u32 val = FIFOValue(MMU.fifos + fifonum);
				u32 remote = (proc+1) & 1;
				u16 IPCFIFO_CNT_remote = ((u16 *)(MMU.MMU_MEM[remote][0x40]))[0x184>>1];
				IPCFIFO_CNT |= (MMU.fifos[fifonum].empty<<8) | (MMU.fifos[fifonum].full<<9) | (MMU.fifos[fifonum].error<<14);
				IPCFIFO_CNT_remote |= (MMU.fifos[fifonum].empty) | (MMU.fifos[fifonum].full<<1);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1] = IPCFIFO_CNT;
				((u16 *)(MMU.MMU_MEM[remote][0x40]))[0x184>>1] = IPCFIFO_CNT_remote;
				if(MMU.fifos[fifonum].empty)
					MMU.reg_IF[proc] |=  ((IPCFIFO_CNT & (1<<2))<<15);// & (MMU.reg_IME[proc]<<17);// & (MMU.reg_IE[proc] & (1<<17));// 
				return val;
				}
			}
			return 0;
			case 0x04000100 :
			case 0x04000104 :
			case 0x04000108 :
			case 0x0400010C :
			{
				u32 val = (((u16 *)(MMU.MMU_MEM[proc][0x40]))[((adr+2)&0xFFF)>>1]);
				return MMU.timer[proc][(adr&0xF)>>2] | (val<<16);
			}	
			case 0x04000640 :	/* TODO (clear): again, ??? */
				LOG("read proj\r\n");
			return 0;
			case 0x04000680 :
				LOG("read roat\r\n");
			return 0;
			case 0x04000620 :
				LOG("point res\r\n");
			return 0;
			
			case CARD_DATA_RD:
			{
				if(!MMU.dscard[proc].adress) return 0;
				
				//u32 val = ((u32 *)MMU.CART_ROM)[MMU.dscard[proc].adress >> 2];
				u32 val = ROM_32(MMU.CART_ROM, MMU.dscard[proc].adress); /* get data from rom */
				
				MMU.dscard[proc].adress += 4;	/* increment adress */
				
				MMU.dscard[proc].transfer_count--;	/* update transfer counter */
				if(MMU.dscard[proc].transfer_count) /* if transfer is not ended */
				{
					return val;	/* return data */
				}
				else	/* transfer is done */
				{
					MEM_32(MMU.MMU_MEM[proc], CARD_CR2) &= ~(CARD_DATA_READY | CARD_ACTIVATE);	/* we're done, edit control register */
					/* = 0x7f7fffff */
					
					/* if needed, throw irq for the end of transfer */
					if(MEM_16(MMU.MMU_MEM[proc], CARD_CR1) & CARD_CR1_IRQ)
					{
						if(proc == ARMCPU_ARM7)	NDS_makeARM7Int(IRQ_CARD); 
						else NDS_makeARM9Int(IRQ_CARD);
					}
					
					return val;
				}
			}

			default :
				break;
		}
	}
	
	/* Returns data from memory */
	return ((u32 *)(MMU.MMU_MEM[proc][(adr>>20)&0xFF]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>2];
}
	
void FASTCALL MMU_write8(u32 proc, u32 adr, u8 val)
{
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Writes data in DTCM (ARM9 only) */
		ARM9Mem.ARM9_DTCM[adr&0x3FFF] = val;
		return ;
	}
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
		cflash_write(adr,val);
		return;
	}
	
	adr &= 0x0FFFFFFF;
	
	switch(adr)
	{
		/* TODO: EEEK ! Controls for VRAMs A, B, C, D are missing ! */
		/* TODO: Not all mappings of VRAMs are handled... (especially BG and OBJ modes) */
		case VRAM_E_CR :
			if(proc == ARMCPU_ARM9)
			{
				if((val & 7) == VRAM_E_BG_EXT_PALETTE)
				{
					ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x80000;
					ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x82000;
					ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x84000;
					ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x86000;
				}
				else if((val & 7) == VRAM_E_TEX_PALETTE)
				{
					ARM9Mem.texPalSlot[0] = ARM9Mem.ARM9_LCD + 0x80000;
					ARM9Mem.texPalSlot[1] = ARM9Mem.ARM9_LCD + 0x82000;
					ARM9Mem.texPalSlot[2] = ARM9Mem.ARM9_LCD + 0x84000;
					ARM9Mem.texPalSlot[3] = ARM9Mem.ARM9_LCD + 0x86000;
				}
			}
			break;
		
		case VRAM_F_CR :
			if(proc == ARMCPU_ARM9)
			{
				switch(val & 0x1F)
				{
					case VRAM_F_BG_EXT_PALETTE : 
						ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x90000;
						ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x92000;
						break;
						
					case VRAM_F_BG_EXT_PALETTE | VRAM_OFFSET(1) :
						ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x90000;
						ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x92000;
						break;
						
					case VRAM_F_TEX_PALETTE :
						ARM9Mem.texPalSlot[0] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
					case VRAM_F_TEX_PALETTE | VRAM_OFFSET(1) :
						ARM9Mem.texPalSlot[1] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
					case VRAM_F_TEX_PALETTE | VRAM_OFFSET(2) :
						ARM9Mem.texPalSlot[2] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
					case VRAM_F_TEX_PALETTE | VRAM_OFFSET(3) :
						ARM9Mem.texPalSlot[3] = ARM9Mem.ARM9_LCD + 0x90000;
						break;
						
					case VRAM_F_OBJ_EXT_PALETTE :
					case VRAM_F_OBJ_EXT_PALETTE | VRAM_OFFSET(1) :
					case VRAM_F_OBJ_EXT_PALETTE | VRAM_OFFSET(2) :
					case VRAM_F_OBJ_EXT_PALETTE | VRAM_OFFSET(3) :
						ARM9Mem.ObjExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x90000;
						ARM9Mem.ObjExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x92000;
						break;
				}
		 	}
			break;
		case VRAM_G_CR :
			if(proc == ARMCPU_ARM9)
			{
		 		switch(val & 0x1F)
				{
					case VRAM_G_BG_EXT_PALETTE : 
						ARM9Mem.ExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x94000;
						ARM9Mem.ExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x96000;
						break;
						
					case VRAM_G_BG_EXT_PALETTE | VRAM_OFFSET(1) :
						ARM9Mem.ExtPal[0][2] = ARM9Mem.ARM9_LCD + 0x94000;
						ARM9Mem.ExtPal[0][3] = ARM9Mem.ARM9_LCD + 0x96000;
						break;
						
					case VRAM_G_TEX_PALETTE :
						ARM9Mem.texPalSlot[0] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
					case VRAM_G_TEX_PALETTE | VRAM_OFFSET(1) :
						ARM9Mem.texPalSlot[1] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
					case VRAM_G_TEX_PALETTE | VRAM_OFFSET(2) :
						ARM9Mem.texPalSlot[2] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
					case VRAM_G_TEX_PALETTE | VRAM_OFFSET(3) :
						ARM9Mem.texPalSlot[3] = ARM9Mem.ARM9_LCD + 0x94000;
						break;
						
					case VRAM_G_OBJ_EXT_PALETTE :
					case VRAM_G_OBJ_EXT_PALETTE | VRAM_OFFSET(1) :
					case VRAM_G_OBJ_EXT_PALETTE | VRAM_OFFSET(2) :
					case VRAM_G_OBJ_EXT_PALETTE | VRAM_OFFSET(3) :
						ARM9Mem.ObjExtPal[0][0] = ARM9Mem.ARM9_LCD + 0x94000;
						ARM9Mem.ObjExtPal[0][1] = ARM9Mem.ARM9_LCD + 0x96000;
						break;
				}
			}
			break;
			
		case VRAM_H_CR  :
			if(proc == ARMCPU_ARM9)
			{
				if((val & 7) == VRAM_H_SUB_BG_EXT_PALETTE)
				{
					ARM9Mem.ExtPal[1][0] = ARM9Mem.ARM9_LCD + 0x98000;
					ARM9Mem.ExtPal[1][1] = ARM9Mem.ARM9_LCD + 0x9A000;
					ARM9Mem.ExtPal[1][2] = ARM9Mem.ARM9_LCD + 0x9C000;
					ARM9Mem.ExtPal[1][3] = ARM9Mem.ARM9_LCD + 0x9E000;
				}
			}
			break;
			
		case VRAM_I_CR  :
			if(proc == ARMCPU_ARM9)
			{
				if((val & 7) == VRAM_I_SUB_SPRITE_EXT_PALETTE)
				{
					ARM9Mem.ObjExtPal[1][0] = ARM9Mem.ARM9_LCD + 0xA0000;
					ARM9Mem.ObjExtPal[1][1] = ARM9Mem.ARM9_LCD + 0xA2000;
				}
			}
			break;
			
#ifdef LOG_CARD 
		case 0x040001A0 : /* TODO (clear): ??? */
		case 0x040001A1 :
		case 0x040001A2 :
		case 0x040001A8 :
		case 0x040001A9 :
		case 0x040001AA :
		case 0x040001AB :
		case 0x040001AC :
		case 0x040001AD :
		case 0x040001AE :
		case 0x040001AF :
                    LOG("%08X : %02X\r\n", adr, val);
#endif
		
		default :
			break;
	}
	
	MMU.MMU_MEM[proc][(adr>>20)&0xFF][adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF]]=val;
}

u16 partie = 1;

void FASTCALL MMU_write16(u32 proc, u32 adr, u16 val)
{
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == MMU.DTCMRegion))
	{
		/* Writes in DTCM (ARM9 only) */
		((u16 *)ARM9Mem.ARM9_DTCM)[(adr&0x3FFF)>>1] = val;
		return;
	}
	
	// CFlash writing, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	{
		cflash_write(adr,val);
		return;
	}
		
	adr &= 0x0FFFFFFF;

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			#ifdef RENDER3D
			case 0x04000610 :
				if(proc == ARMCPU_ARM9)
				{
					LOG("CUT DEPTH %08X\r\n", val);
				}
				return;
			case 0x04000340 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glAlphaFunc(val);
				}
				return;
			case 0x04000060 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glControl(val);
				}
				return;
			case 0x04000354 :
				if(proc == ARMCPU_ARM9)
					OGLRender::glClearDepth(val);
				return;
			#endif
			
			case POWER_CR :
				if(proc == ARMCPU_ARM9)
				{
					if(val & (1<<15))
					{
						LOG("Main core on top\n");
						MainScreen.offset = 0;
						SubScreen.offset = 192;
						//nds.swapScreen();
					}
					else
					{
						LOG("Main core on bottom (%04X)\n", val);
						MainScreen.offset = 192;
						SubScreen.offset = 0;
					}
				}
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x304>>1] = val;
				return;

                        case CARD_CR1:
                                MEM_16(MMU.MMU_MEM[proc], CARD_CR1) = val;
                                AUX_SPI_CNT = val;

                                if (val == 0)
                                   mc_reset_com(&MMU.bupmem);     /* reset backup memory device communication */
				return;
				
                        case CARD_EEPDATA:
                                if(val!=0)
                                {
                                   AUX_SPI_CMD = val & 0xFF;
                                }

                                MEM_16(MMU.MMU_MEM[proc], CARD_EEPDATA) = bm_transfer(&MMU.bupmem, val);        /* transfer data to backup memory chip and receive back */
				return;

			case REG_SPICNT :
				if(proc == ARMCPU_ARM7)
				{
					SPI_CNT = val;
					
                                        //MMU.fw.com == 0; /* reset fw device communication */
					
                                        mc_reset_com(&MMU.fw);     /* reset fw device communication */
                                }
				
				MEM_16(MMU.MMU_MEM[proc], REG_SPICNT) = val;
				return;
				
			case REG_SPIDATA :
				if(proc==ARMCPU_ARM7)
				{
					if(val!=0)
					{
						SPI_CMD = val;
					}
			
					u16 spicnt = MEM_16(MMU.MMU_MEM[proc], REG_SPICNT);
					
					switch(spicnt & 0x300)
					{
						case SPI_DEVICE_POWER :
							break;
							
						case SPI_DEVICE_NVRAM :	/* firmware memory device */
							if(SPI_BAUD_MASK(spicnt) != SPI_BAUD_4MHz) 	/* check SPI baudrate (must be 4mhz) */
							{
								MEM_16(MMU.MMU_MEM[proc], REG_SPIDATA) = 0;
								break;
							}
                                                        MEM_16(MMU.MMU_MEM[proc], REG_SPIDATA) = fw_transfer(&MMU.fw, val);        /* transfer data to fw chip and receive back */
							return;
							
						case SPI_DEVICE_TOUCH:
							switch(SPI_CMD & 0x70)
							{
								case 0x00 :
									val = 0;
									break;
								case 0x10 :
									//execute = FALSE;
									if(SPI_CNT&(1<<11))
									{
										if(partie)
										{
											val = ((nds.touchY<<3)&0x7FF);
											partie = 0;
											//execute = FALSE;
											break;
										}
										val = (nds.touchY>>5);
                                                                                partie = 1;
										break;
									}
									val = ((nds.touchY<<3)&0x7FF);
									partie = 1;
									break;
								case 0x20 :
									val = 0;
									break;
								case 0x30 :
									val = 0;
									break;
								case 0x40 :
									val = 0;
									break;
								case 0x50 :
									if(spicnt & SPI_CONTINUOUS)
									{
										if(partie)
										{
											val = ((nds.touchX<<3)&0x7FF);
											partie = 0;
											break;
										}
										val = (nds.touchX>>5);
										partie = 1;
										break;
									}
									val = ((nds.touchX<<3)&0x7FF);
									partie = 1;
									break;
								case 0x60 :
									val = 0;
									break;
								case 0x70 :
									val = 0;
									break;
							}
							break;
							
						case 0x300 :
							/* NOTICE: Device 3 of SPI is reserved (unused and unusable) */
							break;
					}
				}
				
				MEM_16(MMU.MMU_MEM[proc], REG_SPIDATA) = val;
				return;
				
				/* NOTICE: Perhaps we have to use gbatek-like reg names instead of libnds-like ones ...*/
				
			case BG0_X0 :
				GPU_scrollX(MainScreen.gpu, 0, val);
				return;
			case BG1_X0 :
				GPU_scrollX(MainScreen.gpu, 1, val);
				return;
			case BG2_X0 :
				GPU_scrollX(MainScreen.gpu, 2, val);
				return;
			case BG3_X0 :
				GPU_scrollX(MainScreen.gpu, 3, val);
				return;
			case SUB_BG0_X0 :
				GPU_scrollX(SubScreen.gpu, 0, val);
				return;
			case SUB_BG1_X0 :
				GPU_scrollX(SubScreen.gpu, 1, val);
				return;
			case SUB_BG2_X0 :
				GPU_scrollX(SubScreen.gpu, 2, val);
				return;
			case SUB_BG3_X0 :
				GPU_scrollX(SubScreen.gpu, 3, val);
				return;
			case BG0_Y0 :
				GPU_scrollY(MainScreen.gpu, 0, val);
				return;
			case BG1_Y0 :
				GPU_scrollY(MainScreen.gpu, 1, val);
				return;
			case BG2_Y0 :
				GPU_scrollY(MainScreen.gpu, 2, val);
				return;
			case BG3_Y0 :
				GPU_scrollY(MainScreen.gpu, 3, val);
				return;
			case SUB_BG0_Y0 :
				GPU_scrollY(SubScreen.gpu, 0, val);
				return;
			case SUB_BG1_Y0 :
				GPU_scrollY(SubScreen.gpu, 1, val);
				return;
			case SUB_BG2_Y0 :
				GPU_scrollY(SubScreen.gpu, 2, val);
				return;
			case SUB_BG3_Y0 :
				GPU_scrollY(SubScreen.gpu, 3, val);
				return;
			case BG2_XDX :
				GPU_setPA(MainScreen.gpu, 2, val);
				return;
			case BG2_XDY :
				GPU_setPB(MainScreen.gpu, 2, val);
				return;
			case BG2_YDX :
				GPU_setPC(MainScreen.gpu, 2, val);
				return;
			case BG2_YDY :
				GPU_setPD(MainScreen.gpu, 2, val);
				return;
			case SUB_BG2_XDX :
				GPU_setPA(SubScreen.gpu, 2, val);
				return;
			case SUB_BG2_XDY :
				GPU_setPB(SubScreen.gpu, 2, val);
				return;
			case SUB_BG2_YDX :
				GPU_setPC(SubScreen.gpu, 2, val);
				return;
			case SUB_BG2_YDY :
				GPU_setPD(SubScreen.gpu, 2, val);
				return;
			case BG3_XDX :
				GPU_setPA(MainScreen.gpu, 3, val);
				return;
			case BG3_XDY :
				GPU_setPB(MainScreen.gpu, 3, val);
				return;
			case BG3_YDX :
				GPU_setPC(MainScreen.gpu, 3, val);
				return;
			case BG3_YDY :
				GPU_setPD(MainScreen.gpu, 3, val);
				return;
			case SUB_BG3_XDX :
				GPU_setPA(SubScreen.gpu, 3, val);
				return;
			case SUB_BG3_XDY :
				GPU_setPB(SubScreen.gpu, 3, val);
				return;
			case SUB_BG3_YDX :
				GPU_setPC(SubScreen.gpu, 3, val);
				return;
			case SUB_BG3_YDY :
				GPU_setPD(SubScreen.gpu, 3, val);
				return;
			case BG2_CX :
				GPU_setXL(MainScreen.gpu, 2, val);
				return;
			case BG2_CX + 2 :
				GPU_setXH(MainScreen.gpu, 2, val);
				return;
			case SUB_BG2_CX :
				GPU_setXL(SubScreen.gpu, 2, val);
				return;
			case SUB_BG2_CX + 2 :
				GPU_setXH(SubScreen.gpu, 2, val);
				return;
			case BG3_CX :
				GPU_setXL(MainScreen.gpu, 3, val);
				return;
			case BG3_CX + 2 :
				GPU_setXH(MainScreen.gpu, 3, val);
				return;
			case SUB_BG3_CX :
				GPU_setXL(SubScreen.gpu, 3, val);
				return;
			case SUB_BG3_CX + 2 :
				GPU_setXH(SubScreen.gpu, 3, val);
				return;
			case BG2_CY :
				GPU_setYL(MainScreen.gpu, 2, val);
				return;
			case BG2_CY + 2 :
				GPU_setYH(MainScreen.gpu, 2, val);
				return;
			case SUB_BG2_CY :
				GPU_setYL(SubScreen.gpu, 2, val);
				return;
			case SUB_BG2_CY + 2 :
				GPU_setYH(SubScreen.gpu, 2, val);
				return;
			case BG3_CY :
				GPU_setYL(MainScreen.gpu, 3, val);
				return;
			case BG3_CY + 2 :
				GPU_setYH(MainScreen.gpu, 3, val);
				return;
			case SUB_BG3_CY :
				GPU_setYL(SubScreen.gpu, 3, val);
				return;
			case SUB_BG3_CY + 2 :
				GPU_setYH(SubScreen.gpu, 3, val);
				return;
			case BG0_CR :
				GPULOG("MAIN BG0 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(MainScreen.gpu, 0, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x8>>1] = val;
				return;
			case BG1_CR :
				GPULOG("MAIN BG1 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(MainScreen.gpu, 1, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xA>>1] = val;
				return;
			case BG2_CR :
				GPULOG("MAIN BG2 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(MainScreen.gpu, 2, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xC>>1] = val;
				return;
			case BG3_CR :
				GPULOG("MAIN BG3 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(MainScreen.gpu, 3, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xE>>1] = val;
				return;
			case SUB_BG0_CR :
				GPULOG("SUB BG0 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(SubScreen.gpu, 0, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x1008>>1] = val;
				return;
			case SUB_BG1_CR :
				GPULOG("SUB BG1 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(SubScreen.gpu, 1, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x100A>>1] = val;
				return;
			case SUB_BG2_CR :
				GPULOG("SUB BG2 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(SubScreen.gpu, 2, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x100C>>1] = val;
				return;
			case SUB_BG3_CR :
				GPULOG("SUB BG3 SETPROP 16B %08X\r\n", val);
				GPU_setBGProp(SubScreen.gpu, 3, val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x100E>>1] = val;
				return;
				
			case REG_IME :
				MMU.reg_IME[proc] = val&1;
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x208>>1] = val;
				return;
				
			case REG_IE :
				MMU.reg_IE[proc] = (MMU.reg_IE[proc]&0xFFFF0000) | val;
				return;
			case REG_IE + 2 :
				execute = FALSE;
				MMU.reg_IE[proc] = (MMU.reg_IE[proc]&0xFFFF) | (((u32)val)<<16);
				return;
				
			case REG_IF :
				execute = FALSE;
				MMU.reg_IF[proc] &= (~((u32)val)); 
				return;
			case REG_IF + 2 :
				execute = FALSE;
				MMU.reg_IF[proc] &= (~(((u32)val)<<16));
				return;
				
			case 0x04000180 :
				{
				u32 remote = (proc+1)&1;
				u16 IPCSYNC_remote = ((u16 *)(MMU.MMU_MEM[remote][0x40]))[0x180>>1];
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x180>>1] = (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF);
				((u16 *)(MMU.MMU_MEM[remote][0x40]))[0x180>>1] = (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF);
				MMU.reg_IF[remote] |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU.reg_IME[remote] << 16);// & (MMU.reg_IE[remote] & (1<<16));// 
				//execute = FALSE;
				}
				return;
			case 0x04000184 :
				{
				if(val & 0x4008)
				{
					FIFOInit(MMU.fifos + (IPCFIFO+((proc+1)&1)));
					((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1] = (val & 0xBFF4) | 1;
					((u16 *)(MMU.MMU_MEM[(proc+1)&1][0x40]))[0x184>>1] |= (1<<8);
					MMU.reg_IF[proc] |= ((val & 4)<<15);// & (MMU.reg_IME[proc]<<17);// & (MMU.reg_IE[proc]&0x20000);//
					return;
				}
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1] |= (val & 0xBFF4);
				}
				return;
			case 0x04000100 :
			case 0x04000104 :
			case 0x04000108 :
			case 0x0400010C :
				MMU.timerReload[proc][(adr>>2)&3] = val;
				return;
			case 0x04000102 :
			case 0x04000106 :
			case 0x0400010A :
			case 0x0400010E :
				if(val&0x80)
				{
				if(!(val&4)) MMU.timer[proc][((adr-2)>>2)&0x3] = MMU.timerReload[proc][((adr-2)>>2)&0x3];
				else MMU.timer[proc][((adr-2)>>2)&0x3] = 0;
				}
				MMU.timerON[proc][((adr-2)>>2)&0x3] = val & 0x80;
				switch(val&7)
				{
				case 0 :
					MMU.timerMODE[proc][((adr-2)>>2)&0x3] = 0+1;//proc;
					break;
				case 1 :
					MMU.timerMODE[proc][((adr-2)>>2)&0x3] = 6+1;//proc; 
					break;
				case 2 :
					MMU.timerMODE[proc][((adr-2)>>2)&0x3] = 8+1;//proc;
					break;
				case 3 :
					MMU.timerMODE[proc][((adr-2)>>2)&0x3] = 10+1;//proc;
					break;
				default :
					MMU.timerMODE[proc][((adr-2)>>2)&0x3] = 0xFFFF;
					break;
				}
				if(!(val & 0x80))
				MMU.timerRUN[proc][((adr-2)>>2)&0x3] = FALSE;
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[(adr&0xFFF)>>1] = val;
				return;
			case 0x04000002 : 
				{
				//execute = FALSE;
				u32 v = (((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x0>>2]&0xFFFF)|((u32)val<<16);
				GPU_setVideoProp(MainScreen.gpu, v);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x0>>2] = v;
				}
				return;
			case 0x04000000 :
				{
				u32 v = (((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x0]&0xFFFF0000)|val;
				GPU_setVideoProp(MainScreen.gpu, v);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x0] = v;
				}
				return;
			case 0x04001002 : 
				{
				//execute = FALSE;
				u32 v = (((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x1000>>2]&0xFFFF)|((u32)val<<16);
				GPU_setVideoProp(SubScreen.gpu, v);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x1000>>2] = v;
				}
				return;
			case 0x04001000 :
				{
				u32 v = (((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x1000>>2]&0xFFFF0000)|val;
				GPU_setVideoProp(SubScreen.gpu, v);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x1000>>2] = v;
				}
				return;
			//case 0x020D8460 :
			/*case 0x0235A904 :
				LOG("ECRIRE %d %04X\r\n", proc, val);
				execute = FALSE;*/
				case 0x040000BA :
				{
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma0 %04X\r\n", val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xBA>>1] = val;
				DMASrc[proc][0] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xB0>>2];
				DMADst[proc][0] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xB4>>2];
				u32 v = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xB8>>2];
				MMU.DMAStartTime[proc][0] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][0] = v;
				if(MMU.DMAStartTime[proc][0] == 0)
					MMU_doDMA(proc, 0);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 0, DMASrc[proc][0], DMADst[proc][0], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
				case 0x040000C6 :
				{
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma1 %04X\r\n", val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xC6>>1] = val;
				DMASrc[proc][1] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xBC>>2];
				DMADst[proc][1] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xC0>>2];
				u32 v = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xC4>>2];
				MMU.DMAStartTime[proc][1] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][1] = v;
				if(MMU.DMAStartTime[proc][1] == 0)
					MMU_doDMA(proc, 1);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 1, DMASrc[proc][1], DMADst[proc][1], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
				case 0x040000D2 :
				{
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma2 %04X\r\n", val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xD2>>1] = val;
				DMASrc[proc][2] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xC8>>2];
				DMADst[proc][2] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xCC>>2];
				u32 v = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xD0>>2];
				MMU.DMAStartTime[proc][2] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][2] = v;
				if(MMU.DMAStartTime[proc][2] == 0)
					MMU_doDMA(proc, 2);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 2, DMASrc[proc][2], DMADst[proc][2], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
				case 0x040000DE :
				{
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma3 %04X\r\n", val);
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0xDE>>1] = val;
				DMASrc[proc][3] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xD4>>2];
				DMADst[proc][3] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xD8>>2];
				u32 v = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xDC>>2];
				MMU.DMAStartTime[proc][3] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				MMU.DMACrt[proc][3] = v;
		
				if(MMU.DMAStartTime[proc][3] == 0)
					MMU_doDMA(proc, 3);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 3, DMASrc[proc][3], DMADst[proc][3], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
			//case 0x040001A0 : execute = FALSE;
			default :
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>1]=val;
				return;
		}
	}
	((u16 *)(MMU.MMU_MEM[proc][(adr>>20)&0xFF]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>1]=val;
} 

u32 testval = 0;

void FASTCALL MMU_write32(u32 proc, u32 adr, u32 val)
{
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==MMU.DTCMRegion))
	{
		((u32 *)ARM9Mem.ARM9_DTCM)[(adr&0x3FFF)>>2] = val;
		return ;
	}
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
	   cflash_write(adr,val);
	   return;
	}
		
	adr &= 0x0FFFFFFF;
	
	if((adr>>24)==4)
	{
		switch(adr)
		{
#ifdef RENDER3D
			case 0x040004AC :
			((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x4AC>>2] = val;
			if(proc==ARMCPU_ARM9)
				OGLRender::glTexImage2D(testval, TRUE);
			//execute = FALSE;
			return;
			case 0x040004A8 :
				if(proc==ARMCPU_ARM9)
			{
				OGLRender::glTexImage2D(val, FALSE);
				//execute = FALSE;
				testval = val;
			}
			return;
			case 0x04000488 :
				if(proc==ARMCPU_ARM9)
			{
				OGLRender::glTexCoord(val);
				//execute = FALSE;
			}
			return;
			case 0x0400046C :
				if(proc==ARMCPU_ARM9)
				{
					OGLRender::glScale(val);
				}
				return;
			case 0x04000490 :
				if(proc==ARMCPU_ARM9)
				{
					//GPULOG("VERTEX 10 %d\r\n",val);
				}
				return;
			case 0x04000494 :
				if(proc==ARMCPU_ARM9)
				{
					//GPULOG(printf(txt, "VERTEXY %d\r\n",val);
				}
				return;
			case 0x04000498 :
				if(proc==ARMCPU_ARM9)
				{
					//GPULOG("VERTEXZ %d\r\n",val);
				}
				return;
			case 0x0400049C :
				if(proc==ARMCPU_ARM9)
				{
					//GPULOG("VERTEYZ %d\r\n",val);
				}
				return;
			case 0x04000400 :
				if(proc==ARMCPU_ARM9)
				{
					OGLRender::glCallList(val);
				}
				return;
			case 0x04000450 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glRestore();
				}
				return;
			case 0x04000580 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glViewPort(val);
				}
				return;
			case 0x04000350 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glClearColor(val);
				}
				return;
			case 0x04000440 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glMatrixMode(val);
				}
				return;
			case 0x04000458 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::ML4x4ajouter(val);
				}
				return;
			case 0x0400044C : 
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glStoreMatrix(val);
				}
				return;
			case 0x0400045C :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::ML4x3ajouter(val);
				}
				return;
			case 0x04000444 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glPushMatrix();
				}
				return;
			case 0x04000448 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glPopMatrix(val);
				}
				return;
			case 0x04000470 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::addTrans(val);
				}
				return;
			case 0x04000460 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glMultMatrix4x4(val);
				}
				return;
			case 0x04000464 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glMultMatrix4x3(val);
				}
				return;
			case 0x04000468 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glMultMatrix3x3(val);
				}
				return;
			case 0x04000500 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glBegin(val);
				}
				return;
			case 0x04000504 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glEnd();
				}
				return;
			case 0x04000480 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glColor3b(val);
				}
			return;
			case 0x0400048C :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glVertex3(val);
				}
				return;
			case 0x04000540 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glFlush();
				}
				return;
			case 0x04000454 :
				if(proc == ARMCPU_ARM9)
				{
					OGLRender::glLoadIdentity();
				}
				return;
#endif
			case 0x04000020 :
				GPU_setPAPB(MainScreen.gpu, 2, val);
				return;
			case 0x04000024 :
				GPU_setPCPD(MainScreen.gpu, 2, val);
				return;
			case 0x04001020 :
				GPU_setPAPB(SubScreen.gpu, 2, val);
				return;
			case 0x04001024 :
				GPU_setPCPD(SubScreen.gpu, 2, val);
				return;
			case 0x04000030 :
				GPU_setPAPB(MainScreen.gpu, 3, val);
				return;
			case 0x04000034 :
				GPU_setPCPD(MainScreen.gpu, 3, val);
				return;
			case 0x04001030 :
				GPU_setPAPB(SubScreen.gpu, 3, val);
				return;
			case 0x04001034 :
				GPU_setPCPD(SubScreen.gpu, 3, val);
				return;
			case 0x04000028 :
				GPU_setX(MainScreen.gpu, 2, val);
				return;
			case 0x0400002C :
				GPU_setY(MainScreen.gpu, 2, val);
				return;
			case 0x04001028 :
				GPU_setX(SubScreen.gpu, 2, val);
				return;
			case 0x0400102C :
				GPU_setY(SubScreen.gpu, 2, val);
				return;
			case 0x04000038 :
				GPU_setX(MainScreen.gpu, 3, val);
				return;
			case 0x0400003C :
				GPU_setY(MainScreen.gpu, 3, val);
				return;
			case 0x04001038 :
				GPU_setX(SubScreen.gpu, 3, val);
				return;
			case 0x0400103C :
				GPU_setY(SubScreen.gpu, 3, val);
				return;
			case 0x04000010 :
				GPU_scrollXY(MainScreen.gpu, 0, val);
				return;
			case 0x04000014 :
				GPU_scrollXY(MainScreen.gpu, 1, val);
				return;
			case 0x04000018 :
				GPU_scrollXY(MainScreen.gpu, 2, val);
				return;
			case 0x0400001C :
				GPU_scrollXY(MainScreen.gpu, 3, val);
				return;
			case 0x04001010 :
				GPU_scrollXY(SubScreen.gpu, 0, val);
				return;
			case 0x04001014 :
				GPU_scrollXY(SubScreen.gpu, 1, val);
				return;
			case 0x04001018 :
				GPU_scrollXY(SubScreen.gpu, 2, val);
				return;
			case 0x0400101C :
				GPU_scrollXY(SubScreen.gpu, 3, val);
				return;
			case 0x04000000 :
				GPU_setVideoProp(MainScreen.gpu, val);
				
				GPULOG("MAIN INIT 32B %08X\r\n", val);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x0] = val;
				return;
				
			case 0x04001000 : 
				GPU_setVideoProp(SubScreen.gpu, val);
				GPULOG("SUB INIT 32B %08X\r\n", val);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x1000>>2] = val;
				return;
				
			case REG_IME :
				MMU.reg_IME[proc] = val & 1;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x208>>2] = val;
				return;
				
			case REG_IE :
				MMU.reg_IE[proc] = val;
				return;
			
			case REG_IF :
				MMU.reg_IF[proc] &= (~val); 
				return;
				
			case 0x04000100 :
			case 0x04000104 :
			case 0x04000108 :
			case 0x0400010C :
				MMU.timerReload[proc][(adr>>2)&0x3] = (u16)val;
				if(val&0x800000)
				{
					if(!(val&40000)) MMU.timer[proc][(adr>>2)&0x3] = MMU.timerReload[proc][(adr>>2)&0x3];
					else MMU.timer[proc][(adr>>2)&0x3] = 0;
				}
				MMU.timerON[proc][(adr>>2)&0x3] = val & 0x800000;
				switch((val>>16)&7)
				{
					case 0 :
					MMU.timerMODE[proc][(adr>>2)&0x3] = 0+1;//proc;
					break;
					case 1 :
					MMU.timerMODE[proc][(adr>>2)&0x3] = 6+1;//proc;
					break;
					case 2 :
					MMU.timerMODE[proc][(adr>>2)&0x3] = 8+1;//proc;
					break;
					case 3 :
					MMU.timerMODE[proc][(adr>>2)&0x3] = 10+1;//proc;
					break;
					default :
					MMU.timerMODE[proc][(adr>>2)&0x3] = 0xFFFF;
					break;
				}
				if(!(val & 0x800000))
				{
					MMU.timerRUN[proc][(adr>>2)&0x3] = FALSE;
				}
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[(adr&0xFFF)>>2] = val;
				return;
			case 0x04000298 :
				{
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x298>>2] = val;
					u16 cnt = ((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x280>>1];
					s64 num = 0;
					s64 den = 1;
					s64 res;
					s64 mod;
					switch(cnt&3)
					{
					case 0:
					{
						num = (s64)(((s32 *)(MMU.MMU_MEM[proc][0x40]))[0x290>>2]);
						den = (s64)(((s32 *)(MMU.MMU_MEM[proc][0x40]))[0x298>>2]);
					}
					break;
					case 1:
					{
						num = ((s64*)(MMU.MMU_MEM[proc][0x40]))[0x290>>3];
						den = (s64)(((s32 *)(MMU.MMU_MEM[proc][0x40]))[0x298>>2]);
					}
					break;
					case 2:
					{
						return;
						//num = ((s64*)(MMU.MMU_MEM[proc][0x40]))[0x290>>3];
						//den = ((s64*)(MMU.MMU_MEM[proc][0x40]))[0x298>>3];
					}
					break;
					default: 
						break;
					}
					if(den==0)
					{
						res = 0;
						mod = 0;
						cnt |= 0x4000;
						cnt &= 0x7FFF;
					}
					else
					{
						res = num / den;
						mod = num % den;
						cnt &= 0x3FFF;
					}
					DIVLOG("BOUT1 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
											(u32)(den>>32), (u32)den, 
											(u32)(res>>32), (u32)res);
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2A0>>2] = (u32)res;
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2A4>>2] = (u32)(res>>32);
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2A8>>2] = (u32)mod;
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2AC>>2] = (u32)(mod>>32);
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x280>>2] = cnt;
				}
				return;
			case 0x0400029C :
			{
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x29C>>2] = val;
				u16 cnt = ((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x280>>1];
				s64 num = 0;
				s64 den = 1;
				s64 res;
				s64 mod;
				switch(cnt&3)
				{
				case 0:
				{
					return;//
					//num = (s64)(((s32 *)(MMU.MMU_MEM[proc][0x40]))[0x290>>2]);
					//den = (s64)(((s32 *)(MMU.MMU_MEM[proc][0x40]))[0x298>>2]);
				}
				break;
				case 1:
				{
					return;//
					//num = ((s64*)(MMU.MMU_MEM[proc][0x40]))[0x290>>3];
					//den = (s64)(((s32 *)(MMU.MMU_MEM[proc][0x40]))[0x298>>2]);
				}
				break;
				case 2:
				{
					num = ((s64*)(MMU.MMU_MEM[proc][0x40]))[0x290>>3];
					den = ((s64*)(MMU.MMU_MEM[proc][0x40]))[0x298>>3];
				}
				break;
				default: 
					break;
				}
				if(den==0)
				{
					res = 0;
					mod = 0;
					cnt |= 0x4000;
					cnt &= 0x7FFF;
				}
				else
				{
					res = num / den;
					mod = num % den;
					cnt &= 0x3FFF;
				}
				DIVLOG("BOUT2 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
										(u32)(den>>32), (u32)den, 
										(u32)(res>>32), (u32)res);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2A0>>2] = (u32)res;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2A4>>2] = (u32)(res>>32);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2A8>>2] = (u32)mod;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2AC>>2] = (u32)(mod>>32);
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x280>>2] = cnt;
			}
			return;
			case 0x040002B8 :
				{
					//execute = FALSE;
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B8>>2] = val;
					u16 cnt = ((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x2B0>>1];
					u64 v = 1;
					switch(cnt&1)
					{
					case 0:
						v = (u64)(((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B8>>2]);
						break;
					case 1:
						return;
						//v = ((u64*)(MMU.MMU_MEM[proc][0x40]))[0x2B8>>3];
						//break;
					}
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B4>>2] = (u32)sqrt(v);
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B0>>2] = cnt & 0x7FFF;
					SQRTLOG("BOUT1 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B4>>2]);
				}
				return;
			case 0x040002BC :
				{
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2BC>>2] = val;
					u16 cnt = ((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x2B0>>1];
					u64 v = 1;
					switch(cnt&1)
					{
					case 0:
						return;
						//v = (u64)(((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B8>>2]);
						//break;
					case 1:
						v = ((u64*)(MMU.MMU_MEM[proc][0x40]))[0x2B8>>3];
						break;
					}
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B4>>2] = (u32)sqrt(v);
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B0>>2] = cnt & 0x7FFF;
					SQRTLOG("BOUT2 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x2B4>>2]);
				}
				return;
			case 0x04000180 :
				{
					//execute=FALSE;
					u32 remote = (proc+1)&1;
					u32 IPCSYNC_remote = ((u32 *)(MMU.MMU_MEM[remote][0x40]))[0x180>>2];
					((u32 *)(MMU.MMU_MEM[proc][0x40]))[0x180>>2] = (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF);
					((u32 *)(MMU.MMU_MEM[remote][0x40]))[0x180>>2] = (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF);
					MMU.reg_IF[remote] |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU.reg_IME[remote] << 16);// & (MMU.reg_IE[remote] & (1<<16));//
				}
				return;
			case 0x04000184 :
				if(val & 0x4008)
				{
					FIFOInit(MMU.fifos + (IPCFIFO+((proc+1)&1)));
					((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1] = (val & 0xBFF4) | 1;
					((u16 *)(MMU.MMU_MEM[(proc+1)&1][0x40]))[0x184>>1] |= (1<<8);
					MMU.reg_IF[proc] |= ((val & 4)<<15);// & (MMU.reg_IME[proc] << 17);// & (MMU.reg_IE[proc] & 0x20000);//
					return;
				}
				((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1] = (val & 0xBFF4);
				//execute = FALSE;
				return;
			case 0x04000188 :
				{
					u16 IPCFIFO_CNT = ((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1];
					if(IPCFIFO_CNT&0x8000)
					{
					//if(val==43) execute = FALSE;
					u32 remote = (proc+1)&1;
					u32 fifonum = IPCFIFO+remote;
					FIFOAdd(MMU.fifos + fifonum, val);
					IPCFIFO_CNT = (IPCFIFO_CNT & 0xFFFC) | (MMU.fifos[fifonum].full<<1);
					u16 IPCFIFO_CNT_remote = ((u16 *)(MMU.MMU_MEM[remote][0x40]))[0x184>>1];
					IPCFIFO_CNT_remote = (IPCFIFO_CNT_remote & 0xFCFF) | (MMU.fifos[fifonum].full<<10);
					((u16 *)(MMU.MMU_MEM[proc][0x40]))[0x184>>1] = IPCFIFO_CNT;
					((u16 *)(MMU.MMU_MEM[remote][0x40]))[0x184>>1] = IPCFIFO_CNT_remote;
					//((u32 *)(MMU.MMU_MEM[rote][0x40]))[0x214>>2] = (IPCFIFO_CNT_remote & (1<<10))<<8;
					MMU.reg_IF[remote] |= ((IPCFIFO_CNT_remote & (1<<10))<<8);// & (MMU.reg_IME[remote] << 18);// & (MMU.reg_IE[remote] & 0x40000);//
					//execute = FALSE;
					}
				}
				return;
			case 0x040000B8 :
				//LOG("32 bit dma0 %04X\r\n", val);
				DMASrc[proc][0] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xB0>>2];
				DMADst[proc][0] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xB4>>2];
				MMU.DMAStartTime[proc][0] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][0] = val;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xB8>>2]=val;
				if(MMU.DMAStartTime[proc][0] == 0)
					MMU_doDMA(proc, 0);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 0, DMASrc[proc][0], DMADst[proc][0], 0, ((MMU.DMACrt[proc][0]>>27)&7));
				}
				#endif
				//execute = FALSE;
				return;
				case 0x040000C4 :
				//LOG("32 bit dma1 %04X\r\n", val);
				DMASrc[proc][1] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xBC>>2];
				DMADst[proc][1] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xC0>>2];
				MMU.DMAStartTime[proc][1] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][1] = val;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xC4>>2]=val;
				if(MMU.DMAStartTime[proc][1] == 0)
					MMU_doDMA(proc, 1);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 1, DMASrc[proc][1], DMADst[proc][1], 0, ((MMU.DMACrt[proc][1]>>27)&7));
				}
				#endif
				return;
			case 0x040000D0 :
				//LOG("32 bit dma2 %04X\r\n", val);
				DMASrc[proc][2] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xC8>>2];
				DMADst[proc][2] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xCC>>2];
				MMU.DMAStartTime[proc][2] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][2] = val;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xD0>>2]=val;
				if(MMU.DMAStartTime[proc][2] == 0)
					MMU_doDMA(proc, 2);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 2, DMASrc[proc][2], DMADst[proc][2], 0, ((MMU.DMACrt[proc][2]>>27)&7));
				}
				#endif
				return;
				case 0x040000DC :
				//LOG("32 bit dma3 %04X\r\n", val);
				DMASrc[proc][3] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xD4>>2];
				DMADst[proc][3] = ((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xD8>>2];
				MMU.DMAStartTime[proc][3] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				MMU.DMACrt[proc][3] = val;
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[0xDC>>2]=val;
				if(MMU.DMAStartTime[proc][3] == 0)
					MMU_doDMA(proc, 3);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 3, DMASrc[proc][3], DMADst[proc][3], 0, ((MMU.DMACrt[proc][3]>>27)&7));
				}
				#endif
				return;
			case CARD_CR2 :
				{
					if(MEM_8(MMU.MMU_MEM[proc], CARD_COMMAND) == 0xB7)
					{
						MMU.dscard[proc].adress = (MEM_8(MMU.MMU_MEM[proc], CARD_COMMAND+1) << 24) | (MEM_8(MMU.MMU_MEM[proc], CARD_COMMAND+2) << 16) | (MEM_8(MMU.MMU_MEM[proc], CARD_COMMAND+3) << 8) | (MEM_8(MMU.MMU_MEM[proc], CARD_COMMAND+4));
						MMU.dscard[proc].transfer_count = 0x80;// * ((val>>24)&7));
					}
					else
					{
						LOG("CARD command: %02X\n", MEM_8(MMU.MMU_MEM[proc], CARD_COMMAND));
					}
					
					CARDLOG("%08X : %08X %08X\r\n", adr, val, adresse[proc]);
					val |= CARD_DATA_READY;
					
					if(MMU.dscard[proc].adress == 0)
					{
						val &= ~CARD_ACTIVATE; 
						MEM_32(MMU.MMU_MEM[proc], CARD_CR2) = val;
						return;
					}
					MEM_32(MMU.MMU_MEM[proc], CARD_CR2) = val;
					
					int i;
					
					/* launch DMA if start flag was set to "DS Cart" */
					if(proc == ARMCPU_ARM7) i = 2;
					else i = 5;
					
					if(proc == ARMCPU_ARM9 && MMU.DMAStartTime[proc][0] == i)	/* dma0/1 on arm7 can't start on ds cart event */
					{
						MMU_doDMA(proc, 0);
						return;
					}
					else if(proc == ARMCPU_ARM9 && MMU.DMAStartTime[proc][1] == i)
					{
						MMU_doDMA(proc, 1);
						return;
					}
					else if(MMU.DMAStartTime[proc][2] == i)
					{
						MMU_doDMA(proc, 2);
						return;
					}
					else if(MMU.DMAStartTime[proc][3] == i)
					{
						MMU_doDMA(proc, 3);
						return;
					}
					return;

				}
				return;
			case 0x04000008 :
				GPU_setBGProp(MainScreen.gpu, 0, (val&0xFFFF));
				GPU_setBGProp(MainScreen.gpu, 1, (val>>16));
				//if((val>>16)==0x400) execute = FALSE;
				((u32 *)ARM9Mem.ARM9_REG)[8>>2] = val;
				return;
			case 0x0400000C :
				GPU_setBGProp(MainScreen.gpu, 2, (val&0xFFFF));
				GPU_setBGProp(MainScreen.gpu, 3, (val>>16));
				((u32 *)ARM9Mem.ARM9_REG)[0xC>>2] = val;
				return;
			case 0x04001008 :
				GPU_setBGProp(SubScreen.gpu, 0, (val&0xFFFF));
				GPU_setBGProp(SubScreen.gpu, 1, (val>>16));
				((u32 *)ARM9Mem.ARM9_REG)[0x1008>>2] = val;
				return;
			case 0x0400100C :
				GPU_setBGProp(SubScreen.gpu, 2, (val&0xFFFF));
				GPU_setBGProp(SubScreen.gpu, 3, (val>>16));
				((u32 *)ARM9Mem.ARM9_REG)[0x100C>>2] = val;
				return;
			//case 0x21FDFF0 :  if(val==0) execute = FALSE;
			//case 0x21FDFB0 :  if(val==0) execute = FALSE;
			default :
				((u32 *)(MMU.MMU_MEM[proc][0x40]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>2]=val;
				return;
		}
	}
	((u32 *)(MMU.MMU_MEM[proc][(adr>>20)&0xFF]))[(adr&MMU.MMU_MASK[proc][(adr>>20)&0xFF])>>2]=val;
	
}


void FASTCALL MMU_doDMA(u32 proc, u32 num)
{
	u32 src = DMASrc[proc][num];
	u32 dst = DMADst[proc][num];
	if(src==dst)
	{
	 ((u32 *)(MMU.MMU_MEM[proc][0x40]))[(0xB8 + (0xC*num))>>2] &= 0x7FFFFFFF;
	 return;
	}
	
	if((!(MMU.DMACrt[proc][num]&(1<<31)))&&(!(MMU.DMACrt[proc][num]&(1<<25))))
	{
	 MMU.DMAStartTime[proc][num] = 0;
	 MMU.DMACycle[proc][num] = 0;
	 //MMU.DMAing[proc][num] = FALSE;
	 return;
	}
	
	u32 taille = (MMU.DMACrt[proc][num]&0xFFFF);
	
	if(MMU.DMAStartTime[proc][num] == 5) taille *= 0x80;
	
	MMU.DMACycle[proc][num] = taille + nds.cycles;
	
	MMU.DMAing[proc][num] = TRUE;
	
	DMALOG("proc %d, dma %d src %08X dst %08X start %d taille %d repeat %s %08X\r\n", proc, num, src, dst, MMU.DMAStartTime[proc][num], taille, (MMU.DMACrt[proc][num]&(1<<25))?"on":"off",MMU.DMACrt[proc][num]);
	
	if(!(MMU.DMACrt[proc][num]&(1<<25))) MMU.DMAStartTime[proc][num] = 0;
	
	switch((MMU.DMACrt[proc][num]>>26)&1)
	{
	 case 1 :
	 switch(((MMU.DMACrt[proc][num]>>21)&0xF))
	 {
		  u32 i;
		 case 0 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst += 4;
			  src += 4;
		  }
		  break;		  
		 case 1 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst -= 4;
			  src += 4;
		  }
		  break;
		 case 2 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  src += 4;
		  }
		  break;
		 case 3 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst += 4;
			  src += 4;
		  }
		  break;
		 case 4 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst += 4;
			  src -= 4;
		  }
		  break;
		 case 5 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst -= 4;
			  src -= 4;
		  }
		  break;
		 case 6 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  src -= 4;
		  }
		  break;
		 case 7 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst += 4;
			  src -= 4;
		  }
		  break;
		 case 8 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst += 4;
		  }
		  break;
		 case 9 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst -= 4;
		  }
		  break;
		 case 10 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
		  }
		  break;
		 case 11 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_writeWord(proc, dst, MMU_readWord(proc, src));
			  dst += 4;
		  }
		  break;
		 default :
			break;
	 }
	 break;
	 case 0 :
	 switch(((MMU.DMACrt[proc][num]>>21)&0xF))
	 {
		  u32 i;
		 case 0 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst += 2;
			  src += 2;
		  }
		  break;		  
		 case 1 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst -= 2;
			  src += 2;
		  }
		  break;
		 case 2 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  src += 2;
		  }
		  break;
		 case 3 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst += 2;
			  src += 2;
		  }
		  break;
		 case 4 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst += 2;
			  src -= 2;
		  }
		  break;
		 case 5 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst -= 2;
			  src -= 2;
		  }
		  break;
		 case 6 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  src -= 2;
		  }
		  break;
		 case 7 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst += 2;
			  src -= 2;
		  }
		  break;
		 case 8 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst += 2;
		  }
		  break;
		 case 9 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst -= 2;
		  }
		  break;
		 case 10 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
		  }
		  break;
		 case 11 :
		  for(i = 0; i < taille; ++i)
		  {
			  MMU_write16(proc, dst, MMU_readHWord(proc, src));
			  dst += 2;
		  }
		  break;
		 default :
			break;
	 }
	 break;
	}

	//MMU.DMACrt[proc][num] &= 0x7FFFFFFF;
	//((u32 *)(MMU.MMU_MEM[proc][0x40]))[(0xB8 + (0xC*num))>>2] = MMU.DMACrt[proc][num];
}