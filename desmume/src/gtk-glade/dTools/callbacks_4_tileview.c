/* callbacks_3_palview.c - this file is part of DeSmuME
 *
 * Copyright (C) 2007 Damien Nozay (damdoum)
 * Author: damdoum at users.sourceforge.net
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "callbacks_dtools.h"

void init_combo_memory(GtkComboBox *combo, u8 ** addresses) {
	GtkTreeIter iter;
	GtkListStore* model = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_combo_box_set_model(combo, model);
	int i=0;

#define DO(str,addr,r)  \
	gtk_list_store_append (model, &iter); \
	gtk_list_store_set (model, &iter, 0, str,-1); \
	addresses[i]=(addr r); i++;

	DO("A-BG - 0x6000000",ARM9Mem.ARM9_ABG,)
	DO("A-BG - 0x6010000",ARM9Mem.ARM9_ABG,+0x10000)
	DO("A-BG - 0x6020000",ARM9Mem.ARM9_ABG,+0x20000)
	DO("A-BG - 0x6030000",ARM9Mem.ARM9_ABG,+0x30000)
	DO("A-BG - 0x6040000",ARM9Mem.ARM9_ABG,+0x40000)
	DO("A-BG - 0x6050000",ARM9Mem.ARM9_ABG,+0x50000)
	DO("A-BG - 0x6060000",ARM9Mem.ARM9_ABG,+0x60000)
	DO("A-BG - 0x6070000",ARM9Mem.ARM9_ABG,+0x70000)
	
	DO("B-BG - 0x6200000",ARM9Mem.ARM9_BBG,)
	DO("B-BG - 0x6210000",ARM9Mem.ARM9_BBG,+0x10000)
	
	DO("A-OBJ- 0x6400000",ARM9Mem.ARM9_AOBJ,)
	DO("A-OBJ- 0x6410000",ARM9Mem.ARM9_AOBJ,+0x10000)
	DO("A-OBJ- 0x6420000",ARM9Mem.ARM9_AOBJ,+0x20000)
	DO("A-OBJ- 0x6430000",ARM9Mem.ARM9_AOBJ,+0x30000)
	
	DO("B-OBJ- 0x6600000",ARM9Mem.ARM9_BOBJ,)
	DO("B-OBJ- 0x6610000",ARM9Mem.ARM9_BOBJ,+0x10000)
	
	DO("LCD - 0x6800000",ARM9Mem.ARM9_LCD,)
	DO("LCD - 0x6810000",ARM9Mem.ARM9_LCD,+0x10000)
	DO("LCD - 0x6820000",ARM9Mem.ARM9_LCD,+0x20000)
	DO("LCD - 0x6830000",ARM9Mem.ARM9_LCD,+0x30000)
	DO("LCD - 0x6840000",ARM9Mem.ARM9_LCD,+0x40000)
	DO("LCD - 0x6850000",ARM9Mem.ARM9_LCD,+0x50000)
	DO("LCD - 0x6860000",ARM9Mem.ARM9_LCD,+0x60000)
	DO("LCD - 0x6870000",ARM9Mem.ARM9_LCD,+0x70000)
	DO("LCD - 0x6880000",ARM9Mem.ARM9_LCD,+0x80000)
	DO("LCD - 0x6890000",ARM9Mem.ARM9_LCD,+0x90000)
#undef DO
	gtk_combo_box_set_active(combo,0);
}

static u16* pal_addr[20];
static u8* mem_addr[26];
static BOOL init=FALSE;
static int palnum=0;
static int palindex=0;
static int memnum=0;
static int colnum=0;
static void refresh();
static GtkWidget * wPaint;
static GtkSpinButton * wSpin;
static u16 mem[0x100];


static COLOR c;
static COLOR32 c32;
static GdkGC * gdkGC;



static void wtools_4_update() {

}


static void refresh() {

}

static void initialize() {
	GtkComboBox * combo;
	if (init) return;

	wPaint= glade_xml_get_widget(xml_tools, "wDraw_Tile");
	wSpin = (GtkSpinButton*)glade_xml_get_widget(xml_tools, "wtools_4_palnum");
	combo = (GtkComboBox*)glade_xml_get_widget(xml_tools, "wtools_4_palette");
	init_combo_palette(combo, pal_addr);
	combo = (GtkComboBox*)glade_xml_get_widget(xml_tools, "wtools_4_memory");
	init_combo_memory(combo, mem_addr);
	init=TRUE;
}


void     on_wtools_4_TileView_show         (GtkWidget *widget, gpointer data) {
	initialize();
	register_Tool(wtools_4_update);
}
gboolean on_wtools_4_TileView_close         (GtkWidget *widget, ...) {
	unregister_Tool(wtools_4_update);
	gtk_widget_hide(widget);
	return TRUE;
}

void     on_wtools_4_palette_changed      (GtkComboBox *combo,   gpointer user_data) {
	palindex = gtk_combo_box_get_active(combo);
	gtk_widget_set_sensitive((GtkWidget*)wSpin,(palindex >=4));
	gtk_spin_button_set_value(wSpin,0);
	refresh();
}
void     on_wtools_4_palnum_value_changed (GtkSpinButton *spin, gpointer user_data) {
	palnum = gtk_spin_button_get_value_as_int(spin);
	refresh();
}
void     on_wtools_4_memory_changed (GtkComboBox *combo, gpointer user_data) {
	memnum = gtk_combo_box_get_active(combo);
	refresh();
}
void	 on_wtools_4_rXX_toggled (GtkToggleButton *togglebutton, gpointer user_data) {
	colnum = dyn_CAST(int,user_data);
	refresh();
}



#if 0

void other_screen (GtkWidget * widget, int screen) {
	if (!my_gl_Begin(screen)) return;

	my_gl_Identity();
	glClear( GL_COLOR_BUFFER_BIT );
	
	GPU * gpu = &SubScreen;
	_OAM_ * spriteInfo = (_OAM_*)(gpu->oam + 127);// + 127;
	u16 i; int mode;
	u8 prioTab[256];

#define MODE_IDX_16  1
#define MODE_IDX_256 2
#define MODE_BMP     3

	for(i = 0; i<127; ++i, --spriteInfo)
	{
		size sprSize;
		s32 sprX, sprY, x, y, lg;
		int xdir;
		u8 prio, * src;
		u16 * pal;
		u16 i,j;
		u16 rotScaleA,rotScaleB,rotScaleC,rotScaleD;
		int block;

		prio = spriteInfo->Priority;

		// get sprite location and size
		sprX    = (spriteInfo->X<<23)>>23;
		sprY    =  spriteInfo->Y;
		sprSize = sprSizeTab[spriteInfo->Size][spriteInfo->Shape];
	
		lg = sprSize.x;

		if (spriteInfo->RotScale == 2) continue;
#if 0	
		// switch TOP<-->BOTTOM
		if (spriteInfo->VFlip);
		// switch LEFT<-->RIGHT
		if (spriteInfo->HFlip);

		{
			u16 rotScaleIndex;
			// index from 0 to 31
			rotScaleIndex = spriteInfo->RotScalIndex + (spriteInfo->HFlip<<1) + (spriteInfo->VFlip << 2);
			rotScaleA = T1ReadWord((u8*)(gpu->oam + rotScaleIndex*0x20 + 0x06),0) ;
			rotScaleB = T1ReadWord((u8*)(gpu->oam + rotScaleIndex*0x20 + 0x0E),0) ;
			rotScaleC = T1ReadWord((u8*)(gpu->oam + rotScaleIndex*0x20 + 0x16),0) ;
			rotScaleD = T1ReadWord((u8*)(gpu->oam + rotScaleIndex*0x20 + 0x1E),0) ;
		}

		if (spriteInfo->Mode == 2) {
			src = gpu->sprMem + ((spriteInfo->TileIndex)<<5) + ((y>>3)<<10);
			continue;
		}

		if (spriteInfo->Mode == 3)              /* sprite is in BMP format */
		{
			src = (gpu->sprMem) + (spriteInfo->TileIndex<<4) + (y<<gpu->sprBMPBoundary);

			if (gpu->dispCnt.bits.OBJ_BMP_2D_dim) // 256*256
				src = (gpu->sprMem) + (((spriteInfo->TileIndex&0x3F0) * 64  + (spriteInfo->TileIndex&0x0F) *8) << 1);
			else // 128 * 512
				src = (gpu->sprMem) + (((spriteInfo->TileIndex&0x3E0) * 64  + (spriteInfo->TileIndex&0x1F) *8) << 1);
			continue;
		}


        if(gpu->dispCnt.bits.OBJ_Tile_1D)

		if (spriteInfo->Depth) {
			//256 colors
			glColorTable(GL_TEXTURE_COLOR_TABLE_EXT,
				GL_RGBA, 256, GL_RGBA,
				GL_UNSIGNED_SHORT_1_5_5_5_REV, pal);
		} else {
			pal += (spriteInfo->PaletteIndex<<4);
			glColorTable(GL_TEXTURE_COLOR_TABLE_EXT,
				GL_RGBA, 16, GL_RGBA,
				GL_UNSIGNED_SHORT_1_5_5_5_REV, pal);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, COLOR_INDEX4_EXT,
			16, 16, 0,
			GL_COLOR_INDEX, GL_UNSIGNED_BYTE, src);
		src = gpu->sprMem + ((spriteInfo->TileIndex)<<5) + ((y>>3)<<10);
		pal = (u16*)(ARM9Mem.ARM9_VMEM + 0x200 + gpu->core *0x400);
#endif			
	}
	struct _DISPCNT * dispcnt = &(gpu->dispCnt.bits);

	int boundary = 32;
	if (dispcnt->OBJ_Tile_1D)
		boundary <<= dispcnt->OBJ_Tile_1D_Bound;

	int bmpboundary = 128;
	bmpboundary <<= (dispcnt->OBJ_BMP_mapping & dispcnt->OBJ_BMP_1D_Bound);

	guint Textures[3];
	glGenTextures(3, Textures);
	glBindTexture(GL_TEXTURE_2D, Textures[0]);
	//proxy
	glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA,
		256, 256, 0,
		GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, NULL);

	u16 * pal = (u16*)(ARM9Mem.ARM9_VMEM + 0x200 + gpu->core *0x400);

	u8  palette_16[1024][64];
	u8  palette_256[1024][64];
//	u16 tile_BMP[1024][64];
	u8  * index16  = gpu->sprMem;
	u8  * index256 = gpu->sprMem;
	u16 * indexBMP = gpu->sprMem;
	int tile_n, index;
	if (gpu->sprMem != NULL)
	for (tile_n=0; tile_n<1024; tile_n++) {
		for (index=0; index<64; index++) {
//			tile_BMP[tile_n][index]=*indexBMP;
//			indexBMP++;
			palette_256[tile_n][index]=pal[*index256];
			index256++;

			if (index & 1) continue;
			palette_16[tile_n][index]  =pal[*index16 & 15];
			palette_16[tile_n][index+1]=pal[*index16 >> 4];
			index16++;
		}
		glBindTexture(GL_TEXTURE_2D, Textures[0]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 
			(tile_n & 0x1F)<<3, (tile_n >> 5)<<3,
			8, 8, GL_RGBA, 
			GL_UNSIGNED_SHORT_1_5_5_5_REV, indexBMP);
	}

	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); glVertex2d(-1.0, 1.0);
		glTexCoord2f(0.0, 1.0); glVertex2d(-1.0,-1.0);
		glTexCoord2f(1.0, 1.0); glVertex2d( 1.0,-1.0);
		glTexCoord2f(1.0, 0.0); glVertex2d( 1.0, 1.0);
	glEnd();

	my_gl_End(screen);
	glDeleteTextures(3, &Textures);
}

#endif