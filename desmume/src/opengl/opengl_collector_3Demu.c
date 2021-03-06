/* $Id: opengl_collector_3Demu.c,v 1.16 2007-06-27 14:48:07 masscat Exp $
 */
/*  
	Copyright (C) 2006-2007 Ben Jaques, shash

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

/*
 * This is a 3D emulation plugin. It uses OpenGL to perform the rendering.
 * There is no platform specific code. Platform specific code is executed
 * via a set of helper functions that must be defined should a particular
 * platform use this plugin.
 *
 * The NDS 3D commands are collected until the flush command is issued. At this
 * point the OpenGL function calls that correspnd to the set of commands are called.
 * This approach is taken to allowing simple OpenGL context switching should
 * OpenGL also be being used for other purposes (for example rendering the screen).
 */

/*
 * FIXME: This is a Work In Progress
 * - The NDS command set should be checked to to ensure that it corresponds to a
 *   valid OpenGL command sequence.
 * - Two sets of matrices should be maintained (maybe). One for rendering and one
 *   for NDS test commands. Any matrix commands should be executed immediately on
 *   the NDS test matix set (as well as stored).
 * - Most of the OpenGL/emulation stuff has been copied from shash's Windows
 *   3D code. There maybe optimisations and/or problems arising from the change
 *   of approach or the cut and paste.
 * - More of the 3D needs to be emulated (correctly or at all) :).
 */

#ifndef __MINGW32__

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "types.h"

#include "render3D.h"
#include "matrix.h"
#include "MMU.h"
#include "bits.h"

#include "opengl_collector_3Demu.h"

#define LOG_ALWAYS( fmt, ...) fprintf( stdout, "OpenGL Collector: "); \
fprintf( stdout, fmt, ##__VA_ARGS__)

#if 0
#define LOG( fmt, ...) fprintf( stdout, "OpenGL Collector: "); \
fprintf( stdout, fmt, ##__VA_ARGS__)
#else
#define LOG( fmt, ...)
#endif

#if 0
#define LOG_TEXTURE( fmt, ...) fprintf( stdout, "OpenGL Collector texture: "); \
fprintf( stdout, fmt, ##__VA_ARGS__)
#else
#define LOG_TEXTURE( fmt, ...)
#endif

#if 0
#define LOG_CALL_LIST( fmt, ...) fprintf( stdout, "OpenGL Collector: Call list: "); \
fprintf( stdout, fmt, ##__VA_ARGS__)
#else
#define LOG_CALL_LIST( fmt, ...)
#endif

#if 0
#define LOG_ERROR( fmt, ...) fprintf( stdout, "OpenGL Collector error: "); \
fprintf( stdout, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR( fmt, ...)
#endif

#if 0
#define LOG_MATRIX( matrix) \
LOG_ALWAYS( "%f, %f, %f, %f\n", matrix[0], matrix[1], matrix[2], matrix[3]); \
LOG_ALWAYS( "%f, %f, %f, %f\n", matrix[4], matrix[5], matrix[6], matrix[7]); \
LOG_ALWAYS( "%f, %f, %f, %f\n", matrix[8], matrix[9], matrix[10], matrix[11]); \
LOG_ALWAYS( "%f, %f, %f, %f\n", matrix[12], matrix[13], matrix[14], matrix[15])
#else
#define LOG_MATRIX( matrix)
#endif

#define USE_BGR_ORDER 1

/**
 * Define this to use software vertex transformation.
 * The NDS can handle commands to change the modelview matrix during
 * primitive definitions (between the begin and end). OpenGL does
 * not like this.
 * When this is defined the Modelview matrix will be left as the
 * identity matrix and vertices are transformed before being passed
 * to OpenGL.
 */
#define USE_SOFTWARE_VERTEX_TRANSFORM 1

/**
 * Define this to enable Alpha Blending emulation.
 * How the NDS renders transulcent polygons (order) is not fully understood
 * so some polygon may not be rendered or rendered incorrectly.
 */
#define ENABLE_ALPHA_BLENDING 1

/** the largest number of parameters used by any command */
#define MAX_NUMBER_OF_PARMS 32

/* Define this if you want to perform the glReadPixel call
 * immediately after the render completion.
 * If undefined the glReadPixel is performed in get_line_3Dgl_collect
 * when called for line 0.
 *
 * Reading the pixels immediately may cause change of frame during display.
 * Not reading the pixels immediately may mean that a frame is displayed early.
 */
//#define READ_PIXELS_IMMEDIATELY 1

static int
not_set( void) {
  LOG_ERROR( "platform code not setup\n");
  return 0;
}

static void
nothingness( void) {
}

static void
flush_only( void) {
  glFlush();
}

int (*begin_opengl_ogl_collector_platform)( void) = not_set;
void (*end_opengl_ogl_collector_platform)( void) = nothingness;
int (*initialise_ogl_collector_platform)( void) = not_set;
void (*complete_render_ogl_collector_platform)( void) = flush_only;


#define fix2float(v)    (((float)((s32)(v))) / (float)(1<<12))
#define fix10_2float(v) (((float)((s32)(v))) / (float)(1<<9))



static u8  GPU_screen3D[256*256*4]={0};

static u8 stencil_buffer[256*192];

#ifndef READ_PIXELS_IMMEDIATELY
/** flag indicating if the 3D emulation has produced a new render */
static int new_render_available = 0;
#endif

/*
 * The matrices
 */
static int current_matrix_mode = 0;
static MatrixStack	mtxStack[4];
static float		mtxCurrent [4][16];
static float		mtxTemporal[16];

static u32 disp_3D_control = 0;

static u32 textureFormat=0, texturePalette=0;
static u32 lastTextureFormat=0, lastTexturePalette=0;
static unsigned int oglTextureID=0;
static u8 texMAP[1024*2048*4], texMAP2[2048*2048*4];
static float invTexWidth  = 1.f;
static float invTexHeight = 1.f;
static int texCoordinateTransform = 0;
static int t_texture_coord = 0, s_texture_coord = 0;
static GLint colorRGB[4] = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
static float cur_vertex[3] = {0.0f, 0.0f, 0.0f};

static u32 alpha_function = 0;

static float lightDirection[4][4];
static int light_direction_valid = 0;
static GLint lightColor[4][4];
static int light_colour_valid = 0;

static u32 current_depth24b = 0;
static int depth24b_valid = 0;
static u32 current_clear_colour = 0;
static int clear_colour_valid = 0;


/** the current polygon attribute */
static u32 current_polygon_attr = 0;

/** flag set when a primitive is being defined */
static int inside_primitive = 0;
/** the type of primitive being defined */
static GLenum current_primitive_type;



enum command_type {
  NOP_CMD = 0x00,
  MTX_MODE_CMD = 0x10,
  MTX_PUSH_CMD = 0x11,
  MTX_POP_CMD = 0x12,
  MTX_STORE_CMD = 0x13,
  MTX_RESTORE_CMD = 0x14,
  MTX_IDENTITY_CMD = 0x15,
  MTX_LOAD_4x4_CMD = 0x16,
  MTX_LOAD_4x3_CMD = 0x17,
  MTX_MULT_4x4_CMD = 0x18,
  MTX_MULT_4x3_CMD = 0x19,
  MTX_MULT_3x3_CMD = 0x1a,
  MTX_SCALE_CMD = 0x1b,
  MTX_TRANS_CMD = 0x1c,
  COLOR_CMD = 0x20,
  NORMAL_CMD = 0x21,
  TEXCOORD_CMD = 0x22,
  VTX_16_CMD = 0x23,
  VTX_10_CMD = 0x24,
  VTX_XY_CMD = 0x25,
  VTX_XZ_CMD = 0x26,
  VTX_YZ_CMD = 0x27,
  VTX_DIFF_CMD = 0x28,
  POLYGON_ATTR_CMD = 0x29,
  TEXIMAGE_PARAM_CMD = 0x2a,
  PLTT_BASE_CMD = 0x2b,
  DIF_AMB_CMD = 0x30,
  SPE_EMI_CMD = 0x31,
  LIGHT_VECTOR_CMD = 0x32,
  LIGHT_COLOR_CMD = 0x33,
  SHININESS_CMD = 0x34,
  BEGIN_VTXS_CMD = 0x40,
  END_VTXS_CMD = 0x41,
  SWAP_BUFFERS_CMD = 0x50,
  VIEWPORT_CMD = 0x60,
  BOX_TEST_CMD = 0x70,
  POS_TEST_CMD = 0x71,
  VEC_TEST_CMD = 0x72,

  /* The next ones are not NDS commands */
  CLEAR_COLOUR_CMD = 0x80,
  CLEAR_DEPTH_CMD = 0x81,
  FOG_COLOUR_CMD = 0x82,
  FOG_OFFSET_CMD = 0x83,
  CONTROL_CMD = 0x84,
  ALPHA_FUNCTION_CMD = 0x85
};

#define LAST_CMD_VALUE 0x84

#define ADD_RENDER_PARM_CMD( cmd) render_states[current_render_state].cmds[render_states[current_render_state].write_index++] = cmd

static const char *primitive_type_names[] = {
  "Triangles",
  "Quads",
  "Tri strip",
  "Quad strip"
};

// Accelerationg tables
static float float16table[65536];
static float float10Table[1024];
static float float10RelTable[1024];
static float normalTable[1024];
static int numVertex = 0;

#define NUM_RENDER_STATES 2
int current_render_state;
static struct render_state {
  int write_index;

  /* FIXME: how big to make this? */
  u32 cmds[100*1024];

  //int cmds_drawn;
} render_states[NUM_RENDER_STATES];

#define GET_DRAW_STATE_INDEX( current_index) (((current_index) - 1) & (NUM_RENDER_STATES - 1))
#define GET_NEXT_RENDER_STATE_INDEX( current_index) (((current_index) + 1) & (NUM_RENDER_STATES - 1))


struct cmd_processor {
  u32 num_parms;
  void (*processor_fn)( struct render_state *state,
                        const u32 *parms);
};
/*static int (*cmd_processors[LAST_CMD_VALUE+1])( struct render_state *state,
                                                const u32 *parms);
*/
static struct cmd_processor cmd_processors[LAST_CMD_VALUE+1];







static void
set_gl_matrix_mode( int mode) {
  switch ( mode & 0x3) {
  case 0:
    glMatrixMode( GL_PROJECTION);
    break;

  case 1:
    glMatrixMode( GL_MODELVIEW);
    break;

  case 2:
    /* FIXME: more here? */
    glMatrixMode( GL_MODELVIEW);
    break;

  case 3:
    //glMatrixMode( GL_TEXTURE);
    break;
  }
}

static void
loadMatrix( float *matrix) {
#ifdef USE_SOFTWARE_VERTEX_TRANSFORM
  if ( current_matrix_mode == 0)
#endif
    glLoadMatrixf( matrix);
}


static int
SetupTexture (unsigned int format, unsigned int palette) {
  int alpha_texture = 0;
  if(format == 0) // || disableTexturing)
    {
      LOG("Texture format is zero\n");
      glDisable (GL_TEXTURE_2D);
      return 0;
    }
  else
    {
      unsigned short *pal = NULL;
      unsigned int sizeX = (1<<(((format>>20)&0x7)+3));
      unsigned int sizeY = (1<<(((format>>23)&0x7)+3));
      unsigned int mode = (unsigned short)((format>>26)&0x7);
      const unsigned char * adr;
      //unsigned short param = (unsigned short)((format>>30)&0xF);
      //unsigned short param2 = (unsigned short)((format>>16)&0xF);
      unsigned int imageSize = sizeX*sizeY;
      unsigned int paletteSize = 0;
      unsigned int palZeroTransparent = BIT29(format) ? 0 : 255;
      unsigned int x=0, y=0;
      u32 bytes_in_slot;
      u32 orig_bytes_in_slot;
      int current_slot = (format >> 14) & 3;

      /* Get the address within the texture slot */
      adr = ARM9Mem.textureSlotAddr[current_slot];
      adr += (format&0x3FFF) << 3;

      bytes_in_slot = 0x20000 - ((format & 0x3fff) << 3);
      orig_bytes_in_slot = bytes_in_slot;

      if ( BIT29(format) && mode != 0) {
        alpha_texture = 1;
      }

      LOG_TEXTURE("Texture %08x: size %d by %d\n", format, sizeX, sizeY);

      if (mode == 0)
        glDisable (GL_TEXTURE_2D);
      else
        glEnable (GL_TEXTURE_2D);

      switch(mode)
        {
        case 1:
          {
            alpha_texture = 1;
            paletteSize = 256;
            pal = (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<4));
            break;
          }
        case 2:
          {
            paletteSize = 4;
            pal = (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<3));
            imageSize >>= 2;
            break;
          }
        case 3:
          {
            paletteSize = 16;
            pal = (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<4));
            imageSize >>= 1;
            break;
          }
        case 4:
          {
            paletteSize = 256;
            pal = (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<4));
            break;
          }
        case 5:
          {
            alpha_texture = 1;
            paletteSize = 0;
            pal = (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<4));
            break;
          }
        case 6:
          {
            alpha_texture = 1;
            paletteSize = 256;
            pal = (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<4));
            break;
          }
        case 7:
          {
            alpha_texture = 1;
            paletteSize = 0;
            break;
          }
        }

      //if (!textureCache.IsCached((u8*)pal, paletteSize, adr, imageSize, mode, palZeroTransparent))
      {
        unsigned char * dst = texMAP;// + sizeX*sizeY*3;

        LOG_TEXTURE("Setting up texture mode %d\n", mode);

        switch(mode)
          {
          case 1:
            {
              for(x = 0; x < imageSize; x++, dst += 4)
                {
                  unsigned short c = pal[adr[x]&31], alpha = (adr[x]>>5);
                  dst[0] = (unsigned char)((c & 0x1F)<<3);
                  dst[1] = (unsigned char)((c & 0x3E0)>>2);
                  dst[2] = (unsigned char)((c & 0x7C00)>>7);
                  dst[3] = ((alpha<<2)+(alpha>>1))<<3;
                  if ( dst[3] != 0) {
                    /* full range alpha */
                    dst[3] |= 0x7;
                  }

                  /* transverse to the next texture slot if needs be */
                  bytes_in_slot -= 1;
                  if ( bytes_in_slot == 0) {
                    current_slot += 1;
                    adr = ARM9Mem.textureSlotAddr[current_slot];
                    adr -= orig_bytes_in_slot;
                    bytes_in_slot = 0x20000;
                    orig_bytes_in_slot = bytes_in_slot;
                  }
                }

              break;
            }
          case 2:
            {
              for(x = 0; x < imageSize; ++x)
                {
                  unsigned short c = pal[(adr[x])&0x3];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = ((adr[x]&3) == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  c = pal[((adr[x])>>2)&0x3];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = (((adr[x]>>2)&3) == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  c = pal[((adr[x])>>4)&0x3];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = (((adr[x]>>4)&3) == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  c = pal[(adr[x])>>6];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = (((adr[x]>>6)&3) == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  /* transverse to the next texture slot if needs be */
                  bytes_in_slot -= 1;
                  if ( bytes_in_slot == 0) {
                    current_slot += 1;
                    adr = ARM9Mem.textureSlotAddr[current_slot];
                    adr -= orig_bytes_in_slot;
                    bytes_in_slot = 0x20000;
                    orig_bytes_in_slot = bytes_in_slot;
                  }
                }
            }
            break;
          case 3:
            {
              for(x = 0; x < imageSize; x++)
                {
                  unsigned short c = pal[adr[x]&0xF];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = (((adr[x])&0xF) == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  c = pal[((adr[x])>>4)];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = (((adr[x]>>4)&0xF) == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  /* transverse to the next texture slot if needs be */
                  bytes_in_slot -= 1;
                  if ( bytes_in_slot == 0) {
                    current_slot += 1;
                    adr = ARM9Mem.textureSlotAddr[current_slot];
                    adr -= orig_bytes_in_slot;
                    bytes_in_slot = 0x20000;
                    orig_bytes_in_slot = bytes_in_slot;
                  }
                }
            }
            break;

          case 4:
            {
              for(x = 0; x < imageSize; ++x)
                {
                  unsigned short c = pal[adr[x]];
                  //LOG_TEXTURE("mode 4: x %d colour %04x index %d\n", x, c, adr[x]);
                  dst[0] = (unsigned char)((c & 0x1F)<<3);
                  dst[1] = (unsigned char)((c & 0x3E0)>>2);
                  dst[2] = (unsigned char)((c & 0x7C00)>>7);
                  dst[3] = (adr[x] == 0) ? palZeroTransparent : 255;//(c>>15)*255;
                  dst += 4;

                  /* transverse to the next texture slot if needs be */
                  bytes_in_slot -= 1;
                  if ( bytes_in_slot == 0) {
                    current_slot += 1;
                    adr = ARM9Mem.textureSlotAddr[current_slot];
                    adr -= orig_bytes_in_slot;
                    bytes_in_slot = 0x20000;
                    orig_bytes_in_slot = bytes_in_slot;
                  }
                }
            }
            break;

          case 5:
            {
              unsigned short * pal =
                (unsigned short *)(ARM9Mem.texPalSlot[0] + (texturePalette<<4));
              const unsigned short * slot1;
              const unsigned int * map = (const unsigned int *)adr;
              unsigned int i = 0;
              unsigned int * dst = (unsigned int *)texMAP;

              if ( (format & 0xc000) == 0x8000) {
                /* texel are in slot 2 */
                slot1 =
                  (const unsigned short*)
                  &ARM9Mem.textureSlotAddr[1][((format&0x3FFF)<<2) + 0x10000];
                //LOG_TEXTURE("slot 2 compressed \n");
              }
              else {
                slot1 =
                  (const unsigned short*)
                  &ARM9Mem.textureSlotAddr[1][((format&0x3FFF)<<2) + 0x00000];
                //LOG_TEXTURE("slot 0 compressed\n");
              }

              //LOG_TEXTURE("Compressed texture\n");
              for (y = 0; y < (sizeY/4); y ++)
                {
                  for (x = 0; x < (sizeX/4); x ++, i++)
                    {
                      u32 currBlock	= map[i], sy;
                      u16 pal1		= slot1[i];
                      u16 pal1offset	= (pal1 & 0x3FFF)<<1;
                      u8  mode		= pal1>>14;
                      u32 colours[4];

                      /*
                       * Setup the colours:
                       * In all modes the first two colours are read from
                       * the palette.
                       */
#define RGB16TO32(col,alpha) (((alpha)<<24) | ((((col) & 0x7C00)>>7)<<16) | \
                             ((((col) & 0x3E0)>>2)<<8) | (((col) & 0x1F)<<3))
                      colours[0] = RGB16TO32( pal[pal1offset + 0], 255);
                      colours[1] = RGB16TO32( pal[pal1offset + 1], 255);

                      switch ( mode) {
                      case 0:
                        /* colour 2 is read from the palette
                         * colour 3 is transparent
                         */
                        colours[2] = RGB16TO32( pal[pal1offset+2], 255);
                        colours[3] = RGB16TO32( 0x7fff, 0);
                        break;

                      case 1:
                        /* colour 2 is half colour 0 + half colour 1
                         * colour 3 is transparent
                         */
                        colours[2] =
                          /* RED */
                          (((colours[0] & 0xff) +
                            (colours[1] & 0xff)) >> 1) |
                          /* GREEN */
                          (((colours[0] & (0xff << 8)) +
                            (colours[1] & (0xff << 8))) >> 1) |
                          /* BLUE */
                          (((colours[0] & (0xff << 16)) +
                            (colours[1] & (0xff << 16))) >> 1) | (0xff << 24);
                        colours[3] = RGB16TO32( 0x7fff, 0);
                        break;

                      case 2:
                        /* colour 2 is read from the palette
                         * colour 3 is read from the palette
                         */
                        colours[2] = RGB16TO32( pal[pal1offset+2], 255);
                        colours[3] = RGB16TO32( pal[pal1offset+3], 255);
                        break;

                      case 3: {
                        /* colour 2 is (colour0 * 5 + colour1 * 3) / 8
                         * colour 3 is (colour1 * 5 + colour0 * 3) / 8
                         */
                        u32 red0, red1;
                        u32 green0, green1;
                        u32 blue0, blue1;
                        u16 colour2, colour3;

                        red0 = colours[0] & 0xff;
                        green0 = (colours[0] >> 8) & 0xff;
                        blue0 = (colours[0] >> 16) & 0xff;

                        red1 = colours[1] & 0xff;
                        green1 = (colours[1] >> 8) & 0xff;
                        blue1 = (colours[1] >> 16) & 0xff;

                        colour2 =
                          /* red */
                          ((red0 * 5 + red1 * 3) >> 6) |
                          /* green */
                          (((green0 * 5 + green1 * 3) >> 6) << 5) |
                          /* blue */
                          (((blue0 * 5 + blue1 * 3) >> 6) << 10);
                        colour3 =
                          /* red */
                          ((red1 * 5 + red0 * 3) >> 6) |
                          /* green */
                          (((green1 * 5 + green0 * 3) >> 6) << 5) |
                          /* blue */
                          (((blue1 * 5 + blue0 * 3) >> 6) << 10);

                        colours[2] = RGB16TO32( colour2, 255);
                        colours[3] = RGB16TO32( colour3, 255);
                        break;
                      }
                      }

                      for (sy = 0; sy < 4; sy++)
                        {
                          // Texture offset
                          u32 xAbs = (x<<2);
                          u32 yAbs = ((y<<2) + sy);
                          u32 currentPos = xAbs + yAbs*sizeX;

                          // Palette							
                          u8  currRow		= (u8)((currBlock >> (sy*8)) & 0xFF);

                          dst[currentPos+0] = colours[(currRow >> (2 * 0)) & 3];
                          dst[currentPos+1] = colours[(currRow >> (2 * 1)) & 3];
                          dst[currentPos+2] = colours[(currRow >> (2 * 2)) & 3];
                          dst[currentPos+3] = colours[(currRow >> (2 * 3)) & 3];
                        }

                      /* transverse to the next texture slot if needs be */
                      bytes_in_slot -= 4;
                      if ( bytes_in_slot == 0) {
                        current_slot += 1;
                        map =
                          (const unsigned int *)ARM9Mem.textureSlotAddr[current_slot];
                        map -= orig_bytes_in_slot >> 2;
                        bytes_in_slot = 0x20000;
                        orig_bytes_in_slot = bytes_in_slot;
                      }
                    }
                }
              break;
            }
          case 6:
            {
              for(x = 0; x < imageSize; x++)
                {
                  unsigned short c = pal[adr[x]&7];
                  dst[0] = (unsigned char)((c & 0x1F)<<3);
                  dst[1] = (unsigned char)((c & 0x3E0)>>2);
                  dst[2] = (unsigned char)((c & 0x7C00)>>7);
                  dst[3] = (adr[x]&0xF8);
                  if ( dst[3] != 0) {
                    /* full range alpha */
                    dst[3] |= 0x7;
                  }
                  dst += 4;

                  /* transverse to the next texture slot if needs be */
                  bytes_in_slot -= 1;
                  if ( bytes_in_slot == 0) {
                    current_slot += 1;
                    adr = ARM9Mem.textureSlotAddr[current_slot];
                    adr -= orig_bytes_in_slot;
                    bytes_in_slot = 0x20000;
                    orig_bytes_in_slot = bytes_in_slot;
                  }
                }
              break;
            }
          case 7:
            {
              const unsigned short * map = ((const unsigned short *)adr);
              for(x = 0; x < imageSize; ++x)
                {
                  unsigned short c = map[x];
                  dst[0] = ((c & 0x1F)<<3);
                  dst[1] = ((c & 0x3E0)>>2);
                  dst[2] = ((c & 0x7C00)>>7);
                  dst[3] = (c>>15)*255;
                  dst += 4;

                  /* transverse to the next texture slot if needs be */
                  bytes_in_slot -= 2;
                  if ( bytes_in_slot == 0) {
                    current_slot += 1;
                    map = (const unsigned short *)ARM9Mem.textureSlotAddr[current_slot];
                    map -= orig_bytes_in_slot >> 1;
                    bytes_in_slot = 0x20000;
                    orig_bytes_in_slot = bytes_in_slot;
                  }
                }
            }
            break;
          }

        glBindTexture(GL_TEXTURE_2D, oglTextureID);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, sizeX, sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, texMAP);
        //textureCache.SetTexture ( texMAP, sizeX, sizeY, (u8*)pal, paletteSize, adr, imageSize, mode, palZeroTransparent);

	/*
          switch ((format>>18)&3)
          {
          case 0:
          {
          textureCache.SetTexture ( texMAP, sizeX, sizeY, (u8*)pal, paletteSize, adr, imageSize, mode);
          break;
          }

          case 1:
          {
          u32 *src = (u32*)texMAP, *dst = (u32*)texMAP2;

          for (int y = 0; y < sizeY; y++)
          {
          for (int x = 0; x < sizeX; x++)
          {
          dst[y*sizeX*2 + x] = dst[y*sizeX*2 + (sizeX*2-x-1)] = src[y*sizeX + x];
          }
          }

          sizeX <<= 1;
          textureCache.SetTexture ( texMAP2, sizeX, sizeY, (u8*)pal, paletteSize, adr, imageSize, mode);
          break;
          }

          case 2:
          {
          u32 *src = (u32*)texMAP;

          for (int y = 0; y < sizeY; y++)
          {
          memcpy (&src[(sizeY*2-y-1)*sizeX], &src[y*sizeX], sizeX*4);
          }

          sizeY <<= 1;
          textureCache.SetTexture ( texMAP, sizeX, sizeY, (u8*)pal, paletteSize, adr, imageSize, mode);
          break;
          }

          case 3:
          {
          u32 *src = (u32*)texMAP, *dst = (u32*)texMAP2;

          for (int y = 0; y < sizeY; y++)
          {
          for (int x = 0; x < sizeX; x++)
          {
          dst[y*sizeX*2 + x] = dst[y*sizeX*2 + (sizeX*2-x-1)] = src[y*sizeX + x];
          }
          }

          sizeX <<= 1;

          for (int y = 0; y < sizeY; y++)
          {
          memcpy (&dst[(sizeY*2-y-1)*sizeX], &dst[y*sizeX], sizeX*4);
          }

          sizeY <<= 1;
          textureCache.SetTexture ( texMAP2, sizeX, sizeY, (u8*)pal, paletteSize, adr, imageSize, mode);
          break;
          }
          }
	*/
      }

      invTexWidth  = 1.f/((float)sizeX*(1<<4));//+ 1;
      invTexHeight = 1.f/((float)sizeY*(1<<4));//+ 1;
		
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glMatrixMode (GL_TEXTURE);
      glLoadIdentity ();
      glScaled (invTexWidth, invTexHeight, 1.f);

      set_gl_matrix_mode( current_matrix_mode);

      // S Coordinate options
      if (!BIT16(format))
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
      else {
        if ( BIT18(format)) {
          glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
        }
        else {
          glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        }
      }

      // T Coordinate options
      if (!BIT17(format))
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
      else {
        if ( BIT19(format)) {
          glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
        }
        else {
          glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        }
      }

      texCoordinateTransform = (format>>30);
    }

  return alpha_texture;
}


static void
setup_mode23_tex_coord( float *vertex) {
  float *textureMatrix = mtxCurrent[3]; 
  int s2 =	(int)((	vertex[0] * textureMatrix[0] +  
                        vertex[1] * textureMatrix[4] +  
                        vertex[2] * textureMatrix[8]) + s_texture_coord);
  int t2 =	(int)((	vertex[0] * textureMatrix[1] +  
                        vertex[1] * textureMatrix[5] +  
                        vertex[2] * textureMatrix[9]) + t_texture_coord);

  /*LOG_TEXTURE("mode23 texcoord %d,%d -> %d,%d\n",
              s_texture_coord, t_texture_coord,
              s2, t2);*/

  glTexCoord2i( s2, t2);
}


static INLINE void
setup_vertex( float *vertex) {
#ifdef USE_SOFTWARE_VERTEX_TRANSFORM
  float transformed_vertex[4];
#endif

  LOG("vertex %f,%f,%f\n", vertex[0], vertex[1], vertex[2]);

  if (texCoordinateTransform == 3)
    {
      setup_mode23_tex_coord( vertex);
    }

#ifdef USE_SOFTWARE_VERTEX_TRANSFORM
  /*
   * Transform the vertex
   */
  transformed_vertex[0] = vertex[0];
  transformed_vertex[1] = vertex[1];
  transformed_vertex[2] = vertex[2];
  transformed_vertex[3] = 1.0f;
  MatrixMultVec4x4( mtxCurrent[1], transformed_vertex);

  glVertex4fv( transformed_vertex);
#else
  glVertex3fv( vertex);
#endif

  numVertex += 1;
}


static INLINE void
handle_polygon_attribute( u32 attr, u32 control) {
  static const int texEnv[4] = { GL_MODULATE, GL_DECAL, GL_MODULATE, GL_MODULATE };
  static const int depthFunc[2] = { GL_LESS, GL_EQUAL };
  static const unsigned short map3d_cull[4] = {GL_FRONT_AND_BACK, GL_FRONT, GL_BACK, 0};
  u32 light_mask = attr & 0xf;
  u32 cullingMask;
  GLint colorAlpha;

  //LOG_ALWAYS("poly attr %08x control %08x\n", attr, control);
  /*LOG_ALWAYS("id %d alpha %d mode %d\n",
             (attr >> 24) & 0x3f,
             (attr >> 16) & 0x1f,
             (attr >> 4) & 0x3);*/

  /*
   * lighting
   * (should be done at the normal?)
   */
  if ( light_mask) {
    if ( light_mask & 1) {
      LOG("enabling light 0\n");
      glEnable (GL_LIGHT0);
    }
    else {
      LOG("disabling light 0\n");
      glDisable(GL_LIGHT0);
    }
    if ( light_mask & 2) {
      LOG("enabling light 1\n");
      glEnable (GL_LIGHT1);
    }
    else {
      LOG("disabling light 1\n");
      glDisable(GL_LIGHT1);
    }
    if ( light_mask & 4) {
      LOG("enabling light 2\n");
      glEnable (GL_LIGHT2);
    }
    else {
      LOG("disabling light 2\n");
      glDisable(GL_LIGHT2);
    }
    if ( light_mask & 8) {
      LOG("enabling light 3\n");
      glEnable (GL_LIGHT3);
    }
    else {
      LOG("disabling light 3\n");
      glDisable(GL_LIGHT3);
    }

    glEnable (GL_LIGHTING);
    //glEnable (GL_COLOR_MATERIAL);
  }
  else {
    LOG( "Disabling lighting\n");
    glDisable (GL_LIGHTING);
  }

  /*
   * texture environment
   */
  glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnv[(attr & 0x30)>>4]);

  /*
   * depth function
   */
  glDepthFunc( depthFunc[BIT14(attr)]);

  /*
   * Cull face
   */
  cullingMask = (attr & 0xC0);
  if (cullingMask != 0xC0) {
    glEnable(GL_CULL_FACE);
    glCullFace(map3d_cull[cullingMask>>6]);
  } 
  else {
    glDisable(GL_CULL_FACE);
  }

  /*
   * Alpha value, actually not well handled, 0 should be wireframe
   *
   * FIXME: How are translucent polygons rendered?
   * some use of polygon ID?
   */
  glDepthMask( GL_TRUE);
  colorAlpha = (attr>>16) & 0x1F;
  if ( colorAlpha != 0) {
    colorAlpha = (colorAlpha << 26) | 0x3ffffff;
#if 0
    if ( colorAlpha != 0x7fffffff) {
      if ( !BIT11( attr)) {
        glDepthMask( GL_FALSE);
      }
    }
#endif
    colorRGB[3] = colorAlpha;
    glColor4iv (colorRGB);
  }


  /*
   * Alpha blending
   */
#ifdef ENABLE_ALPHA_BLENDING
  if ( BIT3(control)) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND);
  }
  else {
    glDisable( GL_BLEND);
  }
#endif
}



static void
process_begin_vtxs( struct render_state *state,
                    const u32 *parms) {
  u32 prim_type = parms[0] & 0x3;
  static int alpha_texture = 0;

  LOG("Begin: %s\n", primitive_type_names[prim_type]);

  if ( inside_primitive) {
    LOG( "implicit primitive end\n");
    glEnd();
  }

  if ( depth24b_valid) {
    glClearDepth( current_depth24b / ((float)(1<<24)));
    depth24b_valid = 0;
  }

  if ( clear_colour_valid) {
    glClearColor( ((float)( current_clear_colour & 0x1F))/31.0f,
                  ((float)(( current_clear_colour >> 5)&0x1F))/31.0f, 
                  ((float)(( current_clear_colour >> 10)&0x1F))/31.0f, 
                  ((float)(( current_clear_colour >> 16)&0x1F))/31.0f);
    clear_colour_valid = 0;
  }

  if ( light_colour_valid) {
    int light;

    for ( light = 0; light < 4; light++) {
      if ( light_colour_valid & (1 << light)) {
        glLightiv (GL_LIGHT0 + light, GL_AMBIENT, lightColor[light]);
        glLightiv (GL_LIGHT0 + light, GL_DIFFUSE, lightColor[light]);
        glLightiv (GL_LIGHT0 + light, GL_SPECULAR, lightColor[light]);
      }
    }
    light_colour_valid = 0;
  }

  if ( light_direction_valid) {
    int light;

    for ( light = 0; light < 4; light++) {
      if ( light_direction_valid & (1 << light)) {
        glLightfv(GL_LIGHT0 + light, GL_POSITION, lightDirection[light]);
      }
    }
    light_direction_valid = 0;
  }

#if 1
  /* FIXME: ignoring shadow polygons for now */
  if ( ((current_polygon_attr >> 4) & 0x3) == 3) {
    return;
  }
#endif

  if( disp_3D_control & (1<<2))
    {
      LOG("Alpha test enabled\n");
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc (GL_GREATER, (alpha_function&31)/31.f);
    }
  else
    {
      glDisable(GL_ALPHA_TEST);
    }


  /*
   * Setup for the current polygon attribute
   */
  handle_polygon_attribute( current_polygon_attr, disp_3D_control);


  /* setup the texture */
#if 0
  glDisable (GL_TEXTURE_2D);
#else
  if ( disp_3D_control & 0x1) {
    if (textureFormat  != lastTextureFormat ||
        texturePalette != lastTexturePalette)
      {
        LOG("Setting up texture %08x\n", textureFormat);
        alpha_texture = SetupTexture (textureFormat, texturePalette);

        lastTextureFormat = textureFormat;
        lastTexturePalette = texturePalette;
      }

#if 1
    /* FIXME: this is a hack to get some transparent textures to work */
    if ( alpha_texture) {
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GREATER, 0.0f);
    }
#endif
      
  }
  else {
    glDisable (GL_TEXTURE_2D);
  }
#endif

  inside_primitive = 1;
  switch (prim_type) {
  case 0:
    current_primitive_type = GL_TRIANGLES;
    glBegin( GL_TRIANGLES);
    break;
  case 1:
    current_primitive_type = GL_QUADS;
    glBegin( GL_QUADS);
    break;
  case 2:
    current_primitive_type = GL_TRIANGLE_STRIP;
    glBegin( GL_TRIANGLE_STRIP);
    break;
  case 3:
    current_primitive_type = GL_QUAD_STRIP;
    glBegin( GL_QUAD_STRIP);
    break;
  }
}

static void
process_end_vtxs( struct render_state *state,
                  const u32 *parms) {
  LOG("End\n");
  if ( inside_primitive) {
    glEnd();
  }
  else {
    LOG("End whilst not inside primitive\n");
  }
  inside_primitive = 0;
}

static void
process_viewport( struct render_state *state,
                  const u32 *parms) {
  LOG("Viewport %d,%d,%d,%d\n", parms[0] & 0xff, (parms[0] >> 8) & 0xff,
      (parms[0] >> 16) & 0xff, (parms[0] >> 24) & 0xff);
  glViewport( (parms[0]&0xFF), ((parms[0]>>8)&0xFF),
              ((parms[0]>>16)&0xFF) + 1, (parms[0]>>24) + 1);

}

static void
process_polygon_attr( struct render_state *state,
                      const u32 *parms) {
  LOG("polygon attr %08x\n", parms[0]);

  /*
   * Save this until the next begin
   */
  current_polygon_attr = parms[0];
}

static void
process_normal( struct render_state *state,
                const u32 *parms) {
  LOG("Normal %08x\n", parms[0]);

  float normal[3] = {
    normalTable[parms[0] &1023],
    normalTable[(parms[0]>>10)&1023],
    normalTable[(parms[0]>>20)&1023]
  };

  if (texCoordinateTransform == 2)
    {
      setup_mode23_tex_coord( normal);
    }

  /* transform the vector */
  MatrixMultVec3x3( mtxCurrent[2], normal);

  glNormal3fv(normal);
}

static void
process_teximage_param( struct render_state *state,
                        const u32 *parms) {
  LOG("texture param %08x\n", parms[0]);

  textureFormat = parms[0];
}

static void
process_pltt_base( struct render_state *state,
                   const u32 *parms) {
  LOG("texture palette base %08x\n", parms[0]);

  texturePalette = parms[0];
}


static void
process_dif_amb( struct render_state *state,
                 const u32 *parms) {
  GLint diffuse[4] = {
    (parms[0] & 0x1F) << 26,
    ((parms[0] >> 5) & 0x1F) << 26,
    ((parms[0] >> 10) & 0x1F) << 26,
    0x7fffffff };
  GLint ambient[4] = {
    ((parms[0] >> 16) & 0x1F) << 26,
    ((parms[0] >> 21) & 0x1F) << 26,
    ((parms[0] >> 26) & 0x1F) << 26,
    0x7fffffff };

  LOG("dif amb %08x\n", parms[0]);

  if ( diffuse[0] != 0)
    diffuse[0] |= 0x3ffffff;
  if ( diffuse[1] != 0)
    diffuse[1] |= 0x3ffffff;
  if ( diffuse[2] != 0)
    diffuse[2] |= 0x3ffffff;

  if ( ambient[0] != 0)
    ambient[0] |= 0x3ffffff;
  if ( ambient[1] != 0)
    ambient[1] |= 0x3ffffff;
  if ( ambient[2] != 0)
    ambient[2] |= 0x3ffffff;


  if (BIT15(parms[0])) {
    colorRGB[0] = diffuse[0];
    colorRGB[1] = diffuse[1];
    colorRGB[2] = diffuse[2];
  }

  glMaterialiv (GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
  glMaterialiv (GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
}


static void
process_spe_emi( struct render_state *state,
                 const u32 *parms) {
  GLint specular[4] = {
    (parms[0]&0x1F) << 26,
    ((parms[0]>>5)&0x1F) << 26,
    ((parms[0]>>10)&0x1F) << 26,
    0x7fffffff };
  GLint emission[4] = {
    ((parms[0]>>16)&0x1F) << 26,
    ((parms[0]>>21)&0x1F) << 26,
    ((parms[0]>>26)&0x1F) << 26,
    0x7fffffff };

  if ( specular[0] != 0)
    specular[0] |= 0x3ffffff;
  if ( specular[1] != 0)
    specular[1] |= 0x3ffffff;
  if ( specular[2] != 0)
    specular[2] |= 0x3ffffff;

  if ( emission[0] != 0)
    emission[0] |= 0x3ffffff;
  if ( emission[1] != 0)
    emission[1] |= 0x3ffffff;
  if ( emission[2] != 0)
    emission[2] |= 0x3ffffff;


  LOG("spe emi %08x\n", parms[0]);

  glMaterialiv (GL_FRONT_AND_BACK, GL_SPECULAR, specular);
  glMaterialiv (GL_FRONT_AND_BACK, GL_EMISSION, emission);
}

static void
process_light_vector( struct render_state *state,
                      const u32 *parms) {
  int light = parms[0] >> 30;
  lightDirection[light][0] = -normalTable[parms[0]&1023];
  lightDirection[light][1] = -normalTable[(parms[0]>>10)&1023];
  lightDirection[light][2] = -normalTable[(parms[0]>>20)&1023];
  lightDirection[light][3] = 0.0f;

  LOG("Light vector %f,%f,%f,%f (%08x)\n",
      lightDirection[light][0], lightDirection[light][1],
      lightDirection[light][2], lightDirection[light][3],
      parms[0]);

  MatrixMultVec3x3( mtxCurrent[2], lightDirection[light]);

  /*
   * FIXME:
   * Delay until the next normal command.
   * Can this change inside a primative?
   */
  if ( inside_primitive) {
    //LOG_ALWAYS( "Light vector change whilst inside primative\n");
    /*
     * Save until the 'begin' command
     */
    light_direction_valid |= 1 << light;
  }
  else {
    glLightfv(GL_LIGHT0 + light, GL_POSITION, lightDirection[light]);
    light_direction_valid &= ~(1 << light);
  }
}

static void
process_light_color( struct render_state *state,
                     const u32 *parms) {
  int light = parms[0] >> 30;
  lightColor[light][0] = ((parms[0]) & 0x1F)<<26;
  lightColor[light][1] = ((parms[0]>> 5)&0x1F)<<26;
  lightColor[light][2] = ((parms[0]>> 10)&0x1F)<<26;
  lightColor[light][3] = 0x7fffffff;

  LOG("Light color %08x\n", parms[0]);

  /*
   * FIXME:
   * Delay until the next normal command.
   * Can this change inside a primative?
   */
  if ( inside_primitive) {
    //LOG_ALWAYS( "Light colour change whilst inside primative\n");
    /*
     * Save until the 'begin' command
     */
    light_colour_valid |= 1 << light;
  }
  else {
    glLightiv (GL_LIGHT0 + light, GL_AMBIENT, lightColor[light]);
    glLightiv (GL_LIGHT0 + light, GL_DIFFUSE, lightColor[light]);
    glLightiv (GL_LIGHT0 + light, GL_SPECULAR, lightColor[light]);
    light_colour_valid &= ~(1 << light);
  }
}


static void
process_texcoord( struct render_state *state,
                        const u32 *parms) {
  LOG("texture coord %08x\n", parms[0]);

  t_texture_coord = (s16)(parms[0] >> 16);
  s_texture_coord = (s16)(parms[0] & 0xFFFF);

  if ( texCoordinateTransform == 1)
    {
      float *textureMatrix = mtxCurrent[3];
      int s2 =(int)( s_texture_coord * textureMatrix[0] +
                     t_texture_coord * textureMatrix[4] +
                     (1.f/16.f) * textureMatrix[8] +
                     (1.f/16.f) * textureMatrix[12]);
      int t2 =(int)( s_texture_coord * textureMatrix[1] +
                     t_texture_coord * textureMatrix[5] +
                     (1.f/16.f) * textureMatrix[9] +
                     (1.f/16.f) * textureMatrix[13]);

      /*LOG_TEXTURE("texture coord (pre-trans) s=%d t=%d\n",
                  s_texture_coord, t_texture_coord);
      LOG_MATRIX( textureMatrix);
      LOG_TEXTURE("texture coord (trans) s=%d t=%d\n", s2, t2);*/
      glTexCoord2i (s2, t2);
    }
  else
    {
      /*LOG_TEXTURE("texture coord s=%d t=%d\n", s_texture_coord, t_texture_coord);*/
      glTexCoord2i (s_texture_coord,t_texture_coord);
    }
}

static void
process_mtx_mode( struct render_state *state,
                  const u32 *parms) {
  LOG("Set current matrix %08x\n", parms[0]);

  current_matrix_mode = parms[0] & 0x3;

  set_gl_matrix_mode( current_matrix_mode);
}


static void
process_mtx_identity( struct render_state *state,
                      const u32 *parms) {
  LOG("Load identity (%d)\n", current_matrix_mode);

  MatrixIdentity (mtxCurrent[current_matrix_mode]);

  if (current_matrix_mode == 2)
    MatrixIdentity (mtxCurrent[1]);

  if ( current_matrix_mode < 3)
    glLoadIdentity();
}

static void
process_mtx_push( struct render_state *state,
                  const u32 *parms) {
  LOG("Matrix push\n");

  if ( current_matrix_mode == 2) {
    /* copy the stack position and size from the Model matrix stack
     * as they are shared and may have been updated whilst Model
     * Matrix mode was selected. */
    mtxStack[2].position = mtxStack[1].position;
    mtxStack[2].size = mtxStack[1].size;
  }

  MatrixStackPushMatrix (&mtxStack[current_matrix_mode],
                         mtxCurrent[current_matrix_mode]);

  if ( current_matrix_mode == 2) {
    /* push the model matrix as well */
    MatrixStackPushMatrix ( &mtxStack[1],
                            mtxCurrent[1]);
  }
}

static void
process_mtx_pop( struct render_state *state,
                 const u32 *parms) {
  s32 index = parms[0];
  LOG("Matrix pop\n");

  if ( current_matrix_mode == 2) {
    /* copy the stack position and size from the Model matrix stack
     * as they are shared and may have been updated whilst Model
     * Matrix mode was selected. */
    mtxStack[2].position = mtxStack[1].position;
    mtxStack[2].size = mtxStack[1].size;
  }

  MatrixCopy ( mtxCurrent[current_matrix_mode],
               MatrixStackPopMatrix (&mtxStack[current_matrix_mode], index));

  if (current_matrix_mode == 2) {
    MatrixCopy ( mtxCurrent[1],
                 MatrixStackPopMatrix( &mtxStack[1], index));
  }

  if ( current_matrix_mode < 3) {
    if ( current_matrix_mode == 2)
      loadMatrix( mtxCurrent[1]);
    else
      loadMatrix( mtxCurrent[current_matrix_mode]);
  }
}

static void
process_mtx_store( struct render_state *state,
                   const u32 *parms) {
  LOG("Matrix store (%d)\n", parms[0]);

  if ( current_matrix_mode == 2) {
    /* copy the stack position and size from the Model matrix stack
     * as they are shared and may have been updated whilst Model
     * Matrix mode was selected. */
    mtxStack[2].position = mtxStack[1].position;
    mtxStack[2].size = mtxStack[1].size;
  }

  MatrixStackLoadMatrix (&mtxStack[current_matrix_mode],
                         parms[0] & 31,
                         mtxCurrent[current_matrix_mode]);

  if ( current_matrix_mode == 2) {
    /* store the model matrix as well */
    MatrixStackLoadMatrix (&mtxStack[1],
                           parms[0] & 31,
                           mtxCurrent[1]);
  }
}

static void
process_mtx_restore( struct render_state *state,
                   const u32 *parms) {
  LOG("Matrix restore\n");

  if ( current_matrix_mode == 2) {
    /* copy the stack position and size from the Model matrix stack
     * as they are shared and may have been updated whilst Model
     * Matrix mode was selected. */
    mtxStack[2].position = mtxStack[1].position;
    mtxStack[2].size = mtxStack[1].size;
  }

  MatrixCopy (mtxCurrent[current_matrix_mode],
              MatrixStackGetPos(&mtxStack[current_matrix_mode], parms[0]&31));

  if (current_matrix_mode == 2) {
    MatrixCopy (mtxCurrent[1],
                MatrixStackGetPos(&mtxStack[1], parms[0]&31));
  }

  if ( current_matrix_mode < 3) {
    if ( current_matrix_mode == 2) {
      loadMatrix( mtxCurrent[1]);
    }
    else {
      loadMatrix( mtxCurrent[current_matrix_mode]);
    }
  }
}

static void
process_mtx_load_4x4( struct render_state *state,
                      const u32 *parms) {
  int i;
  LOG("Load 4x4 (%d):\n", current_matrix_mode);
  LOG("%08x, %08x, %08x, %08x\n", parms[0], parms[1], parms[2], parms[3]);
  LOG("%08x, %08x, %08x, %08x\n", parms[4], parms[5], parms[6], parms[7]);
  LOG("%08x, %08x, %08x, %08x\n", parms[8], parms[9], parms[10], parms[11]);
  LOG("%08x, %08x, %08x, %08x\n", parms[12], parms[13], parms[14], parms[15]);

  for ( i = 0; i < 16; i++) {
    mtxCurrent[current_matrix_mode][i] = fix2float(parms[i]);
  }

  if (current_matrix_mode == 2)
    MatrixCopy (mtxCurrent[1], mtxCurrent[2]);

  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][0], mtxCurrent[current_matrix_mode][1],
      mtxCurrent[current_matrix_mode][2], mtxCurrent[current_matrix_mode][3]);
  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][4], mtxCurrent[current_matrix_mode][5],
      mtxCurrent[current_matrix_mode][6], mtxCurrent[current_matrix_mode][7]);
  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][8], mtxCurrent[current_matrix_mode][9],
      mtxCurrent[current_matrix_mode][10], mtxCurrent[current_matrix_mode][11]);
  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][12], mtxCurrent[current_matrix_mode][13],
      mtxCurrent[current_matrix_mode][14], mtxCurrent[current_matrix_mode][15]);

  if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}

static void
process_mtx_load_4x3( struct render_state *state,
                      const u32 *parms) {
  int i;

  LOG("Load 4x3 (%d):\n", current_matrix_mode);
  LOG("%08x, %08x, %08x, 0.0\n", parms[0], parms[1], parms[2]);
  LOG("%08x, %08x, %08x, 0.0\n", parms[3], parms[4], parms[5]);
  LOG("%08x, %08x, %08x, 0.0\n", parms[6], parms[7], parms[8]);
  LOG("%08x, %08x, %08x, 1.0\n", parms[9], parms[10], parms[11]);

  mtxCurrent[current_matrix_mode][3] = mtxCurrent[current_matrix_mode][7] =
    mtxCurrent[current_matrix_mode][11] = 0.0f;
  mtxCurrent[current_matrix_mode][15] = 1.0f;
  mtxCurrent[current_matrix_mode][0] = fix2float(parms[0]);
  mtxCurrent[current_matrix_mode][1] = fix2float(parms[1]);
  mtxCurrent[current_matrix_mode][2] = fix2float(parms[2]);
  mtxCurrent[current_matrix_mode][4] = fix2float(parms[3]);
  mtxCurrent[current_matrix_mode][5] = fix2float(parms[4]);
  mtxCurrent[current_matrix_mode][6] = fix2float(parms[5]);
  mtxCurrent[current_matrix_mode][8] = fix2float(parms[6]);
  mtxCurrent[current_matrix_mode][9] = fix2float(parms[7]);
  mtxCurrent[current_matrix_mode][10] = fix2float(parms[8]);
  mtxCurrent[current_matrix_mode][12] = fix2float(parms[9]);
  mtxCurrent[current_matrix_mode][13] = fix2float(parms[10]);
  mtxCurrent[current_matrix_mode][14] = fix2float(parms[11]);

  if (current_matrix_mode == 2)
    MatrixCopy (mtxCurrent[1], mtxCurrent[2]);

  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][0], mtxCurrent[current_matrix_mode][1],
      mtxCurrent[current_matrix_mode][2], mtxCurrent[current_matrix_mode][3]);
  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][4], mtxCurrent[current_matrix_mode][5],
      mtxCurrent[current_matrix_mode][6], mtxCurrent[current_matrix_mode][7]);
  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][8], mtxCurrent[current_matrix_mode][9],
      mtxCurrent[current_matrix_mode][10], mtxCurrent[current_matrix_mode][11]);
  LOG("%f, %f, %f, %f\n",
      mtxCurrent[current_matrix_mode][12], mtxCurrent[current_matrix_mode][13],
      mtxCurrent[current_matrix_mode][14], mtxCurrent[current_matrix_mode][15]);

  if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}


static void
process_mtx_trans( struct render_state *state,
                   const u32 *parms) {
  float trans[3];

  trans[0] = fix2float(parms[0]);
  trans[1] = fix2float(parms[1]);
  trans[2] = fix2float(parms[2]);

  LOG("translate %lf,%lf,%lf (%d)\n", trans[0], trans[1], trans[2],
      current_matrix_mode);

  MatrixTranslate (mtxCurrent[current_matrix_mode], trans);

  if (current_matrix_mode == 2)
    MatrixTranslate (mtxCurrent[1], trans);

  if ( current_matrix_mode == 2) {
    LOG_MATRIX( mtxCurrent[1]);
    loadMatrix( mtxCurrent[1]);
  }
  else if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}

static void
process_mtx_scale( struct render_state *state,
                   const u32 *parms) {
  float scale[3];

  LOG("scale %08x, %08x, %08x (%d)\n", parms[0], parms[1], parms[2],
      current_matrix_mode);

  scale[0] = fix2float(parms[0]);
  scale[1] = fix2float(parms[1]);
  scale[2] = fix2float(parms[2]);

  if (current_matrix_mode == 2) {
    /* scaling does not happen against the drirection matrix */
    MatrixScale (mtxCurrent[1], scale);
  }
  else
    MatrixScale (mtxCurrent[current_matrix_mode], scale);

  LOG("scale %f,%f,%f\n", scale[0], scale[1], scale[2]);

  if ( current_matrix_mode == 2)
    loadMatrix( mtxCurrent[1]);
  else if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}

static void
process_mtx_mult_4x4( struct render_state *state,
                      const u32 *parms) {
  static float mult_matrix[16];
  int i;
  LOG("Mult 4x4 (%d):\n", current_matrix_mode);

  for ( i = 0; i < 16; i++) {
    mult_matrix[i] = fix2float(parms[i]);
  }

  LOG_MATRIX( mult_matrix);

  MatrixMultiply (mtxCurrent[current_matrix_mode], mult_matrix);

  if (current_matrix_mode == 2)
    MatrixMultiply (mtxCurrent[1], mult_matrix);

  if ( current_matrix_mode == 2)
    loadMatrix( mtxCurrent[1]);
  else if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}

static void
process_mtx_mult_4x3( struct render_state *state,
                      const u32 *parms) {
  static float mult_matrix[16];

  LOG("Mult 4x3 (%d):\n", current_matrix_mode);

  mult_matrix[3] = mult_matrix[7] =
    mult_matrix[11] = 0.0f;
  mult_matrix[15] = 1.0f;
  mult_matrix[0] = fix2float(parms[0]);
  mult_matrix[1] = fix2float(parms[1]);
  mult_matrix[2] = fix2float(parms[2]);
  mult_matrix[4] = fix2float(parms[3]);
  mult_matrix[5] = fix2float(parms[4]);
  mult_matrix[6] = fix2float(parms[5]);
  mult_matrix[8] = fix2float(parms[6]);
  mult_matrix[9] = fix2float(parms[7]);
  mult_matrix[10] = fix2float(parms[8]);
  mult_matrix[12] = fix2float(parms[9]);
  mult_matrix[13] = fix2float(parms[10]);
  mult_matrix[14] = fix2float(parms[11]);

  LOG_MATRIX( mult_matrix);

  MatrixMultiply (mtxCurrent[current_matrix_mode], mult_matrix);

  if (current_matrix_mode == 2)
    MatrixMultiply (mtxCurrent[1], mult_matrix);

  if ( current_matrix_mode == 2)
    loadMatrix( mtxCurrent[1]);
  else if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}

static void
process_mtx_mult_3x3( struct render_state *state,
                      const u32 *parms) {
  static float mult_matrix[16];

  LOG("Mult 3x3 (%d):\n", current_matrix_mode);

  mult_matrix[3] = mult_matrix[7] =
    mult_matrix[11] = 0.0f;
  mult_matrix[12] = mult_matrix[13] =
    mult_matrix[14] = 0.0f;
  mult_matrix[15] = 1.0f;
  mult_matrix[0] = fix2float(parms[0]);
  mult_matrix[1] = fix2float(parms[1]);
  mult_matrix[2] = fix2float(parms[2]);
  mult_matrix[4] = fix2float(parms[3]);
  mult_matrix[5] = fix2float(parms[4]);
  mult_matrix[6] = fix2float(parms[5]);
  mult_matrix[8] = fix2float(parms[6]);
  mult_matrix[9] = fix2float(parms[7]);
  mult_matrix[10] = fix2float(parms[8]);

  LOG_MATRIX( mult_matrix);
  
  MatrixMultiply (mtxCurrent[current_matrix_mode], mult_matrix);

  if (current_matrix_mode == 2)
    MatrixMultiply (mtxCurrent[1], mult_matrix);

  if ( current_matrix_mode == 2)
    loadMatrix( mtxCurrent[1]);
  else if ( current_matrix_mode < 3)
    loadMatrix( mtxCurrent[current_matrix_mode]);
}

static void
process_colour( struct render_state *state,
                const u32 *parms) {
  colorRGB[0] = (parms[0] & 0x1f) << 26;
  colorRGB[1] = ((parms[0] >> 5) & 0x1f) << 26;
  colorRGB[2] = ((parms[0] >> 10) & 0x1f) << 26;

  if ( colorRGB[0] != 0)
    colorRGB[0] |= 0x3ffffff;
  if ( colorRGB[1] != 0)
    colorRGB[1] |= 0x3ffffff;
  if ( colorRGB[2] != 0)
    colorRGB[2] |= 0x3ffffff;

  LOG("colour %08x,%08x,%08x (%08x)\n",
      colorRGB[0], colorRGB[1], colorRGB[2],
      parms[0]);
  glColor4iv( colorRGB);
}

static void
process_vtx_16( struct render_state *state,
                const u32 *parms) {
  cur_vertex[0] = float16table[parms[0] & 0xFFFF];
  cur_vertex[1] = float16table[parms[0] >> 16];
  cur_vertex[2] = float16table[parms[1] & 0xFFFF];
  LOG("vtx16 %08x %08x\n", parms[0], parms[1]);

  setup_vertex( cur_vertex);
}

static void
process_vtx_10( struct render_state *state,
                const u32 *parms) {
  cur_vertex[0] = float10Table[(parms[0]) & 0x3ff];
  cur_vertex[1] = float10Table[(parms[0] >> 10) & 0x3ff];
  cur_vertex[2] = float10Table[(parms[0] >> 20) & 0x3ff];
  LOG("vtx10 %08x\n", parms[0]);

  setup_vertex( cur_vertex);
}

static void
process_vtx_xy( struct render_state *state,
                const u32 *parms) {
  cur_vertex[0] = float16table[(parms[0]) & 0xffff];
  cur_vertex[1] = float16table[(parms[0] >> 16) & 0xffff];
  LOG("vtx xy %08x\n", parms[0]);

  setup_vertex( cur_vertex);
}

static void
process_vtx_xz( struct render_state *state,
                const u32 *parms) {
  cur_vertex[0] = float16table[(parms[0]) & 0xffff];
  cur_vertex[2] = float16table[(parms[0] >> 16) & 0xffff];
  LOG("vtx xz %08x\n", parms[0]);

  setup_vertex( cur_vertex);
}

static void
process_vtx_yz( struct render_state *state,
                const u32 *parms) {
  cur_vertex[1] = float16table[(parms[0]) & 0xffff];
  cur_vertex[2] = float16table[(parms[0] >> 16) & 0xffff];
  LOG("vtx yz %08x\n", parms[0]);

  setup_vertex( cur_vertex);
}

static void
process_vtx_diff( struct render_state *state,
                  const u32 *parms) {
  cur_vertex[0] += float10RelTable[(parms[0]) & 0x3ff];
  cur_vertex[1] += float10RelTable[(parms[0] >> 10) & 0x3ff];
  cur_vertex[2] += float10RelTable[(parms[0] >> 20) & 0x3ff];
  LOG("vtx10 %08x\n", parms[0]);

  setup_vertex( cur_vertex);
}


static void
process_control( struct render_state *state,
                 const u32 *parms) {
  LOG("Control set to %08x\n", parms[0]);

  /*
   * Save for later.
   * all control is handled at a 'begin' command
   */
  disp_3D_control = parms[0];
}

static void
process_alpha_function( struct render_state *state,
                        const u32 *parms) {
  LOG("Alpha function %08x\n", parms[0]);
  /*
   * Save for later
   */
  alpha_function = parms[0];
}

static void
process_clear_depth( struct render_state *state,
                     const u32 *parms) {
  //u32 depth24b;
  u32 v = parms[0] & 0x7FFF;
  LOG("Clear depth %08x\n", parms[0]);

  /*
   * Save for later.
   * handled at a 'begin' command
   */
  current_depth24b = (v * 0x200) + ( (v+1) & 0x8000 ? 0x1ff : 0);

  if ( inside_primitive) {
    depth24b_valid = 1;
  }
  else {
    depth24b_valid = 0;
    glClearDepth( current_depth24b / ((float)(1<<24)));
  }
}

static void
process_clear_colour( struct render_state *state,
                      const u32 *parms) {
  u32 v = parms[0];
  LOG("Clear colour %08x\n", v);

  /*
   * Save for later.
   * handled at a 'begin' command
   */
  current_clear_colour = v;

  if ( inside_primitive) {
    clear_colour_valid = 1;
  }
  else {
    glClearColor( ((float)( current_clear_colour & 0x1F))/31.0f,
                  ((float)(( current_clear_colour >> 5)&0x1F))/31.0f, 
                  ((float)(( current_clear_colour >> 10)&0x1F))/31.0f, 
                  ((float)(( current_clear_colour >> 16)&0x1F))/31.0f);
    clear_colour_valid = 0;
  }
}


/*
 * The rendering function.
 */
static void
draw_3D_area( void) {
  struct render_state *state = &render_states[GET_DRAW_STATE_INDEX(current_render_state)];
  int i;
  GLenum errCode;


  /*** OpenGL BEGIN ***/
  if ( !begin_opengl_ogl_collector_platform()) {
    LOG_ERROR( "platform failed for begin opengl for draw\n");
    return;
  }

#ifndef READ_PIXELS_IMMEDIATELY
  /*
   * If there is a render which has not had its pixels read yet
   * then read them now.
   */
  if ( new_render_available) {
      new_render_available = 0;

#ifdef USE_BGR_ORDER
      glReadPixels(0,0,256,192,GL_BGRA,GL_UNSIGNED_BYTE,GPU_screen3D);
#else
      glReadPixels(0,0,256,192,GL_RGBA,GL_UNSIGNED_BYTE,GPU_screen3D);
#endif
  }
#endif


  LOG("\n------------------------------------\n");
  LOG("Start of render\n");
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  /*if ( state->write_index > 10000) {
    LOG_ALWAYS( "3d write index %d\n", state->write_index);
    }*/

  for ( i = 0; i < state->write_index; i++) {
    u32 cmd = state->cmds[i];
    //LOG("Render cmd: %08x\n", state->cmds[i]);

    if ( cmd < LAST_CMD_VALUE + 1) {
      if ( cmd_processors[cmd].processor_fn != NULL) {
        cmd_processors[cmd].processor_fn( state, &state->cmds[i+1]);
      }
      else {
        LOG_ERROR("Unhandled %02x\n", cmd);
      }
      i += cmd_processors[cmd].num_parms;
    }
  }

  /*
   * Complete any primitive that may be left unended
   */
  if ( inside_primitive) {
    LOG( "implicit primitive end at end\n");
    glEnd();

    inside_primitive = 0;
  }

  complete_render_ogl_collector_platform();

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    const GLubyte *errString;

    errString = gluErrorString(errCode);
    LOG_ERROR( "openGL error during 3D emulation: %s\n", errString);
  }

#ifdef READ_PIXELS_IMMEDIATELY
#ifdef USE_BGR_ORDER
  glReadPixels(0,0,256,192,GL_BGRA,GL_UNSIGNED_BYTE,GPU_screen3D);
#else
  glReadPixels(0,0,256,192,GL_RGBA,GL_UNSIGNED_BYTE,GPU_screen3D);
#endif

  if ((errCode = glGetError()) != GL_NO_ERROR) {
    const GLubyte *errString;

    errString = gluErrorString(errCode);
    LOG_ERROR( "openGL error during glReadPixels: %s\n", errString);
  }
#else
  new_render_available = 1;
#endif


  LOG("End of render\n------------------------------------\n");

  end_opengl_ogl_collector_platform();
  return;
}


static void
init_openGL( void) {
  /* OpenGL BEGIN */
  if ( !begin_opengl_ogl_collector_platform()) {
    LOG_ERROR( "platform failed to begin opengl for initialisation\n");
    return;
  }

  /* Set the background black */
  glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

  /* Enables Depth Testing */
  glEnable( GL_DEPTH_TEST );
  glEnable(GL_TEXTURE_2D);

  glEnable( GL_STENCIL_TEST);

  // Always Passes, 1 Bit Plane, 1 As Mask
  glStencilFunc( GL_ALWAYS, 1, 1);
  glStencilOp( GL_KEEP, GL_REPLACE, GL_REPLACE);

  glGenTextures (1, &oglTextureID);

  glViewport (0, 0, 256, 192);

  end_opengl_ogl_collector_platform();
  /*** OpenGL END ***/
}


static char
nullFunc1_3Dgl_collect(void) {
  return 1;
}

static void
nullFunc2_3Dgl_collect(void) {
}
static void
nullFunc3_3Dgl_collect(unsigned long v) {
}
static void nullFunc4_3Dgl_collect(signed long v){}
static void nullFunc5_3Dgl_collect(unsigned int v){}
static void nullFunc6_3Dgl_collect(unsigned int one,
                                   unsigned int two, unsigned int v){}
static int  nullFunc7_3Dgl_collect(void) {return 0;}
static long nullFunc8_3Dgl_collect(unsigned int index){ return 0; }
static void nullFunc9_3Dgl_collect(int line, unsigned short * DST) { };


static int num_primitives[4];

static char
init_3Dgl_collect( void) {
  int i;

  printf("Initialising 3D renderer for OpenGL Collector\n");

  if ( !initialise_ogl_collector_platform()) {
    printf( "Platform initialisation failed\n");
    return 0;
  }

  MatrixStackSetMaxSize(&mtxStack[0], 1);		// Projection stack
  MatrixStackSetMaxSize(&mtxStack[1], 31);	// Coordinate stack
  MatrixStackSetMaxSize(&mtxStack[2], 31);	// Directional stack
  MatrixStackSetMaxSize(&mtxStack[3], 1);		// Texture stack

  MatrixInit (mtxCurrent[0]);
  MatrixInit (mtxCurrent[1]);
  MatrixInit (mtxCurrent[2]);
  MatrixInit (mtxCurrent[3]);
  MatrixInit (mtxTemporal);

  current_render_state = 0;
  for ( i = 0; i < NUM_RENDER_STATES; i++) {
    render_states[i].write_index = 0;

  }

  for ( i = 0; i < 4; i++) {
    num_primitives[i] = 0;
  }

  for ( i = 0; i < LAST_CMD_VALUE+1; i++) {
    cmd_processors[i].processor_fn = NULL;
    cmd_processors[i].num_parms = 0;

    switch ( i) {
    case NOP_CMD:
      cmd_processors[i].num_parms = 0;
      break;
    case MTX_MODE_CMD:
      cmd_processors[i].processor_fn = process_mtx_mode;
      cmd_processors[i].num_parms = 1;
      break;
    case MTX_PUSH_CMD:
      cmd_processors[i].processor_fn = process_mtx_push;
      cmd_processors[i].num_parms = 0;
      break;
    case MTX_POP_CMD:
      cmd_processors[i].processor_fn = process_mtx_pop;
      cmd_processors[i].num_parms = 1;
      break;
    case MTX_STORE_CMD:
      cmd_processors[i].processor_fn = process_mtx_store;
      cmd_processors[i].num_parms = 1;
      break;
    case MTX_RESTORE_CMD:
      cmd_processors[i].processor_fn = process_mtx_restore;
      cmd_processors[i].num_parms = 1;
      break;
    case MTX_IDENTITY_CMD:
      cmd_processors[i].processor_fn = process_mtx_identity;
      cmd_processors[i].num_parms = 0;
      break;
    case MTX_LOAD_4x4_CMD:
      cmd_processors[i].processor_fn = process_mtx_load_4x4;
      cmd_processors[i].num_parms = 16;
      break;
    case MTX_LOAD_4x3_CMD:
      cmd_processors[i].processor_fn = process_mtx_load_4x3;
      cmd_processors[i].num_parms = 12;
      break;
    case MTX_MULT_4x4_CMD:
      cmd_processors[i].processor_fn = process_mtx_mult_4x4;
      cmd_processors[i].num_parms = 16;
      break;
    case MTX_MULT_4x3_CMD:
      cmd_processors[i].processor_fn = process_mtx_mult_4x3;
      cmd_processors[i].num_parms = 12;
      break;
    case MTX_MULT_3x3_CMD:
      cmd_processors[i].processor_fn = process_mtx_mult_3x3;
      cmd_processors[i].num_parms = 9;
      break;
    case MTX_SCALE_CMD:
      cmd_processors[i].processor_fn = process_mtx_scale;
      cmd_processors[i].num_parms = 3;
      break;
    case MTX_TRANS_CMD:
      cmd_processors[i].processor_fn = process_mtx_trans;
      cmd_processors[i].num_parms = 3;
      break;
    case COLOR_CMD:
      cmd_processors[i].processor_fn = process_colour;
      cmd_processors[i].num_parms = 1;
      break;
    case NORMAL_CMD:
      cmd_processors[i].processor_fn = process_normal;
      cmd_processors[i].num_parms = 1;
      break;
    case TEXCOORD_CMD:
      cmd_processors[i].processor_fn = process_texcoord;
      cmd_processors[i].num_parms = 1;
      break;
    case VTX_16_CMD:
      cmd_processors[i].processor_fn = process_vtx_16;
      cmd_processors[i].num_parms = 2;
      break;
    case VTX_10_CMD:
      cmd_processors[i].processor_fn = process_vtx_10;
      cmd_processors[i].num_parms = 1;
      break;
    case VTX_XY_CMD:
      cmd_processors[i].processor_fn = process_vtx_xy;
      cmd_processors[i].num_parms = 1;
      break;
    case VTX_XZ_CMD:
      cmd_processors[i].processor_fn = process_vtx_xz;
      cmd_processors[i].num_parms = 1;
      break;
    case VTX_YZ_CMD:
      cmd_processors[i].processor_fn = process_vtx_yz;
      cmd_processors[i].num_parms = 1;
      break;
    case VTX_DIFF_CMD:
      cmd_processors[i].processor_fn = process_vtx_diff;
      cmd_processors[i].num_parms = 1;
      break;
    case POLYGON_ATTR_CMD:
      cmd_processors[i].processor_fn = process_polygon_attr;
      cmd_processors[i].num_parms = 1;
      break;
    case TEXIMAGE_PARAM_CMD:
      cmd_processors[i].processor_fn = process_teximage_param;
      cmd_processors[i].num_parms = 1;
      break;
    case PLTT_BASE_CMD:
      cmd_processors[i].processor_fn = process_pltt_base;
      cmd_processors[i].num_parms = 1;
      break;
    case DIF_AMB_CMD:
      cmd_processors[i].processor_fn = process_dif_amb;
      cmd_processors[i].num_parms = 1;
      break;
    case SPE_EMI_CMD:
      cmd_processors[i].processor_fn = process_spe_emi;
      cmd_processors[i].num_parms = 1;
      break;
    case LIGHT_VECTOR_CMD:
      cmd_processors[i].processor_fn = process_light_vector;
      cmd_processors[i].num_parms = 1;
      break;
    case LIGHT_COLOR_CMD:
      cmd_processors[i].processor_fn = process_light_color;
      cmd_processors[i].num_parms = 1;
      break;
    case SHININESS_CMD:
      cmd_processors[i].num_parms = 32;
      break;
    case BEGIN_VTXS_CMD:
      cmd_processors[i].processor_fn = process_begin_vtxs;
      cmd_processors[i].num_parms = 1;
      break;
    case END_VTXS_CMD:
      cmd_processors[i].processor_fn = process_end_vtxs;
      cmd_processors[i].num_parms = 0;
      break;
    case SWAP_BUFFERS_CMD:
      cmd_processors[i].num_parms = 1;
      break;
    case VIEWPORT_CMD:
      cmd_processors[i].processor_fn = process_viewport;
      cmd_processors[i].num_parms = 1;
      break;
    case BOX_TEST_CMD:
      cmd_processors[i].num_parms = 3;
      break;
    case POS_TEST_CMD:
      cmd_processors[i].num_parms = 2;
      break;
    case VEC_TEST_CMD:
      cmd_processors[i].num_parms = 1;
      break;

      /* The next ones are not NDS commands */
    case CLEAR_COLOUR_CMD:
      cmd_processors[i].processor_fn = process_clear_colour;
      cmd_processors[i].num_parms = 1;
      break;
    case CLEAR_DEPTH_CMD:
      cmd_processors[i].processor_fn = process_clear_depth;
      cmd_processors[i].num_parms = 1;
      break;

    case FOG_COLOUR_CMD:
      cmd_processors[i].num_parms = 1;
      break;
    case FOG_OFFSET_CMD:
      cmd_processors[i].num_parms = 1;
      break;

    case CONTROL_CMD:
      cmd_processors[i].processor_fn = process_control;
      cmd_processors[i].num_parms = 1;
      break;

    case ALPHA_FUNCTION_CMD:
      cmd_processors[i].processor_fn = process_alpha_function;
      cmd_processors[i].num_parms = 1;
      break;

    }
  }

  for (i = 0; i < 65536; i++)
    {
      float16table[i] = fix2float((signed short)i);
    }

  for (i = 0; i < 1024; i++)
    {
      float10RelTable[i] = ((signed short)(i<<6)) / (float)(1<<18);
      float10Table[i] = ((signed short)(i<<6)) / (float)(1<<12);
      normalTable[i] = ((signed short)(i<<6)) / (float)(1<<16);
    }

  init_openGL();

  return 1;
}




static void
Flush_3Dgl_collect( unsigned long val) {
  int new_render_state = GET_NEXT_RENDER_STATE_INDEX(current_render_state);
  //struct render_state *state = &render_states[current_render_state];
  struct render_state *new_state = &render_states[new_render_state];

  LOG("Flush %lu %d %d\n", val,
      state->write_index, render_states[new_render_state].write_index);

  current_render_state = new_render_state;

  draw_3D_area();
  new_state->write_index = 0;

  numVertex = 0;
  //LOG("End of render\n");
}

static void
SwapScreen_3Dgl_collect( unsigned int screen) {
  LOG("NEVER USED? SwapScreen %d\n", screen);
}


static void
call_list_3Dgl_collect(unsigned long v) {
  static u32 call_list_command = 0;
  static u32 total_num_parms = 0;
  static u32 current_parm = 0;
  static u32 parms[MAX_NUMBER_OF_PARMS];

  LOG_CALL_LIST("call list - %08x (%08x cur %d tot %d)\n", v,
                call_list_command, current_parm, total_num_parms);

  if ( call_list_command == 0) {
    /* new call list command coming in */
    call_list_command = v;

    total_num_parms = cmd_processors[v & 0xff].num_parms;
    current_parm = 0;

    LOG_CALL_LIST("new command %02x total parms %d\n", v & 0xff,
                  total_num_parms);
  }
  else {
    LOG_CALL_LIST("Adding parm %d - %08x\n", current_parm, v);
    parms[current_parm] = v;
    current_parm += 1;
  }

  if ( current_parm == total_num_parms) {
    do {
      u32 cmd = call_list_command & 0xff;
      LOG_CALL_LIST("Current command is %02x\n", cmd);
      if ( cmd != 0) {
        unsigned int i;

        LOG_CALL_LIST("Cmd %02x complete:\n", cmd);

        if ( cmd == SWAP_BUFFERS_CMD) {
          Flush_3Dgl_collect( parms[0]);
        }
        else {
          /* put the command and parms on the list */
          ADD_RENDER_PARM_CMD( cmd);

          for ( i = 0; i < total_num_parms; i++) {
            ADD_RENDER_PARM_CMD( parms[i]);
            LOG_CALL_LIST("parm %08x\n", parms[i]);
          }
        }
      }
      call_list_command >>=8;

      total_num_parms = cmd_processors[call_list_command & 0xff].num_parms;
      current_parm = 0;

      LOG_CALL_LIST("Next cmd %02x parms %d\n", call_list_command & 0xff,
                    total_num_parms);
    } while ( call_list_command && total_num_parms == 0);
  }
}

static void
viewport_3Dgl_collect(unsigned long v) {
  //LOG("Viewport\n");
  ADD_RENDER_PARM_CMD( VIEWPORT_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
Begin_3Dgl_collect(unsigned long v) {
  //LOG("Begin\n");
  ADD_RENDER_PARM_CMD( BEGIN_VTXS_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
End_3Dgl_collect( void) {
  //LOG("END\n");
  ADD_RENDER_PARM_CMD( END_VTXS_CMD);
}


static void
fog_colour_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( FOG_COLOUR_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
fog_offset_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( FOG_OFFSET_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
clear_depth_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( CLEAR_DEPTH_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
clear_colour_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( CLEAR_COLOUR_CMD);
  ADD_RENDER_PARM_CMD( v);
}



static void
Vertex16b_3Dgl_collect( unsigned int v) {
  static int vertex16_count = 0;
  static u32 parm1;

  if ( vertex16_count == 0) {
    vertex16_count += 1;
    parm1 = v;
  }
  else {
    ADD_RENDER_PARM_CMD( VTX_16_CMD);
    ADD_RENDER_PARM_CMD( parm1);
    ADD_RENDER_PARM_CMD( v);
    vertex16_count = 0;
  }
}

static void
Vertex10b_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( VTX_10_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
Vertex3_cord_3Dgl_collect(unsigned int one, unsigned int two, unsigned int v) {
  if ( one == 0) {
    if ( two == 1) {
      ADD_RENDER_PARM_CMD( VTX_XY_CMD);
    }
    else {
      ADD_RENDER_PARM_CMD( VTX_XZ_CMD);
    }
  }
  else {
    ADD_RENDER_PARM_CMD( VTX_YZ_CMD);
  }
  ADD_RENDER_PARM_CMD( v);
}

static void
Vertex_rel_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( VTX_DIFF_CMD);
  ADD_RENDER_PARM_CMD( v);
}


static void
matrix_mode_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( MTX_MODE_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
load_identity_matrix_3Dgl_collect(void) {
  ADD_RENDER_PARM_CMD( MTX_IDENTITY_CMD);
}


static void
load_4x4_matrix_3Dgl_collect(signed long v) {
  static int count_4x4 = 0;
  static u32 parms[15];

  if ( count_4x4 < 15) {
    parms[count_4x4] = v;
    count_4x4 += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_LOAD_4x4_CMD);

    for ( i = 0; i < 15; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_4x4 = 0;
  }
}

static void
load_4x3_matrix_3Dgl_collect(signed long v) {
  static int count_4x3 = 0;
  static u32 parms[11];

  if ( count_4x3 < 11) {
    parms[count_4x3] = v;
    count_4x3 += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_LOAD_4x3_CMD);

    for ( i = 0; i < 11; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_4x3 = 0;
  }
}


static void
multi_matrix_4x4_3Dgl_collect(signed long v) {
  static int count_4x4 = 0;
  static u32 parms[15];

  if ( count_4x4 < 15) {
    parms[count_4x4] = v;
    count_4x4 += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_MULT_4x4_CMD);

    for ( i = 0; i < 15; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_4x4 = 0;
  }
}

static void
multi_matrix_4x3_3Dgl_collect(signed long v) {
  static int count_4x3 = 0;
  static u32 parms[11];

  if ( count_4x3 < 11) {
    parms[count_4x3] = v;
    count_4x3 += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_MULT_4x3_CMD);

    for ( i = 0; i < 11; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_4x3 = 0;
  }
}

static void
multi_matrix_3x3_3Dgl_collect(signed long v) {
  static int count_3x3 = 0;
  static u32 parms[8];

  if ( count_3x3 < 8) {
    parms[count_3x3] = v;
    count_3x3 += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_MULT_3x3_CMD);

    for ( i = 0; i < 8; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_3x3 = 0;
  }
}




static void
store_matrix_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( MTX_STORE_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
restore_matrix_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( MTX_RESTORE_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
push_matrix_3Dgl_collect( void) {
  ADD_RENDER_PARM_CMD( MTX_PUSH_CMD);
}

static void
pop_matrix_3Dgl_collect(signed long v) {
  ADD_RENDER_PARM_CMD( MTX_POP_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
translate_3Dgl_collect(signed long v) {
  static int count_trans = 0;
  static u32 parms[2];

  if ( count_trans < 2) {
    parms[count_trans] = v;
    count_trans += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_TRANS_CMD);

    for ( i = 0; i < 2; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_trans = 0;
  }
}

static void
scale_3Dgl_collect(signed long v) {
  static int count_scale = 0;
  static u32 parms[2];

  if ( count_scale < 2) {
    parms[count_scale] = v;
    count_scale += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( MTX_SCALE_CMD);

    for ( i = 0; i < 2; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_scale = 0;
  }
}

static void
PolyAttr_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( POLYGON_ATTR_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
TextImage_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( TEXIMAGE_PARAM_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
Colour_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( COLOR_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
material0_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( DIF_AMB_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
material1_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( SPE_EMI_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
shininess_3Dgl_collect(unsigned long v) {
  static int count_shine = 0;
  static u32 parms[31];

  if ( count_shine < 31) {
    parms[count_shine] = v;
    count_shine += 1;
  }
  else {
    int i;

    ADD_RENDER_PARM_CMD( SHININESS_CMD);

    for ( i = 0; i < 31; i++) {
      ADD_RENDER_PARM_CMD( parms[i]);
    }
    ADD_RENDER_PARM_CMD( v);
    count_shine = 0;
  }
}

static void
texture_palette_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( PLTT_BASE_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
texture_coord_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( TEXCOORD_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
light_direction_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( LIGHT_VECTOR_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
light_colour_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( LIGHT_COLOR_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
normal_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( NORMAL_CMD);
  ADD_RENDER_PARM_CMD( v);
}

static void
control_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( CONTROL_CMD);
  ADD_RENDER_PARM_CMD( v);  
}

static void
alpha_function_3Dgl_collect(unsigned long v) {
  ADD_RENDER_PARM_CMD( ALPHA_FUNCTION_CMD);
  ADD_RENDER_PARM_CMD( v);  
}

static int
get_num_polygons_3Dgl_collect( void) {
  /* FIXME: this is a hack */
  LOG("Hacky get_num_polygons %d\n", numVertex/3);

  return numVertex/3;
}
static int
get_num_vertices_3Dgl_collect( void) {
  LOG("get_num_vertices %d\n", numVertex);

  return numVertex;
}

static long
get_clip_matrix_3Dgl_collect(unsigned int index) {
  float val = MatrixGetMultipliedIndex (index, mtxCurrent[0], mtxCurrent[1]);
  LOG("get_clip_matrix %d\n", index);

  val *= (1<<12);

  return (signed long)val;
}

static long
get_direction_matrix_3Dgl_collect(unsigned int index) {
  LOG("get_direction_matrix %d\n", index);
  index += (index/3);

  return (signed long)(mtxCurrent[2][(index)*(1<<12)]);
}


//#define BITMAP_STENCIL 1
static void
get_line_3Dgl_collect(int line, unsigned short *dst) {
  int i;
  u8 *screen3D = (u8 *)&GPU_screen3D[(192-(line%192))*256*4];
#ifdef BITMAP_STENCIL
  u8 *line_stencil = (u8 *)&stencil_buffer[(192-(line%192))*(256>>3)];
#else
  u8 *line_stencil = (u8 *)&stencil_buffer[(192-(line%192))*256];
#endif

#ifndef READ_PIXELS_IMMEDIATELY
  if ( line == 0) {
    if ( new_render_available) {
      new_render_available = 0;
      if ( !begin_opengl_ogl_collector_platform()) {
        LOG_ERROR( "platform failed for begin opengl for get_line\n");
        return;
      }

#ifdef USE_BGR_ORDER
      glReadPixels(0,0,256,192,GL_BGRA,GL_UNSIGNED_BYTE,GPU_screen3D);
#else
      glReadPixels(0,0,256,192,GL_RGBA,GL_UNSIGNED_BYTE,GPU_screen3D);
#endif

#ifdef BITMAP_STENCIL
      glReadPixels(0,0, 256,192, GL_STENCIL_INDEX, GL_BITMAP, stencil_buffer);
#else
      glReadPixels(0,0, 256,192, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil_buffer);
#endif

      end_opengl_ogl_collector_platform();
    }
  }
#endif


#ifdef BITMAP_STENCIL
  for(i = 0; i < 256; i += 8)
    {
      u8 bitmap = *line_stencil++;

      if ( bitmap) {
        if ( bitmap & 0x80) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x40) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x20) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x10) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x08) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x04) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x02) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
        if ( bitmap & 0x01) {
#ifdef USE_BGR_ORDER
          u32 r = screen3D[0], g = screen3D[1], b = screen3D[2], a = screen3D[3];
#else
          u32 r = screen3D[2], g = screen3D[1], b = screen3D[0], a = screen3D[3];
#endif
          r = (r*(a+1)) >> 8; g = (g*(a+1)) >> 8; b = (b*(a+1)) >> 8;
          *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
        }
        dst += 1;
        screen3D += 4;
      }
      else {
        screen3D += 4 * 8;
        dst += 8;
      }
    }
#else
  for(i = 0; i < 256; i++)
    {
      if ( line_stencil[i]) {
#ifdef USE_BGR_ORDER
        u32 r = screen3D[0],
          g = screen3D[1],
          b = screen3D[2],
          a = screen3D[3];
#else
        u32 r = screen3D[2],
          g = screen3D[1],
          b = screen3D[0],
          a = screen3D[3];
#endif
#if 0
        r = (r*a)/255;
        g = (g*a)/255;
        b = (b*a)/255;
#else
        r = (r*(a+1)) >> 8;
        g = (g*(a+1)) >> 8;
        b = (b*(a+1)) >> 8;
#endif

        *dst = (((r>>3)<<10) | ((g>>3)<<5) | (b>>3));
      }
      dst += 1;
      screen3D += 4;
    }
#endif
}


GPU3DInterface gpu3D_opengl_collector = {
  /* the Init function */
  init_3Dgl_collect,

  /* Reset */
  nullFunc2_3Dgl_collect,

  /* Close */
  nullFunc2_3Dgl_collect,

  /* Viewport */
  viewport_3Dgl_collect,

  /* Clear colour */
  clear_colour_3Dgl_collect,

  /* Fog colour */
  fog_colour_3Dgl_collect,

  /* Fog offset */
  fog_offset_3Dgl_collect,

  /* Clear Depth */
  clear_depth_3Dgl_collect,

  /* Matrix Mode */
  matrix_mode_3Dgl_collect,

  /* Load Identity */
  load_identity_matrix_3Dgl_collect,

  /* Load 4x4 Matrix */
  load_4x4_matrix_3Dgl_collect,

  /* Load 4x3 Matrix */
  load_4x3_matrix_3Dgl_collect,

  /* Store Matrix */
  store_matrix_3Dgl_collect,

  /* Restore Matrix */
  restore_matrix_3Dgl_collect,

  /* Push Matrix */
  push_matrix_3Dgl_collect,

  /* Pop Matrix */
  pop_matrix_3Dgl_collect,

  /* Translate */
  translate_3Dgl_collect,

  /* Scale */
  scale_3Dgl_collect,

  /* Multiply Matrix 3x3 */
  multi_matrix_3x3_3Dgl_collect,

  /* Multiply Matrix 4x3 */
  multi_matrix_4x3_3Dgl_collect,

  /* Multiply Matrix 4x4 */
  multi_matrix_4x4_3Dgl_collect,

  /* Begin primitive */
  Begin_3Dgl_collect,
  /* End primitive */
  End_3Dgl_collect,

  /* Colour */
  Colour_3Dgl_collect,

  /* Vertex */
  Vertex16b_3Dgl_collect,
  Vertex10b_3Dgl_collect,
  Vertex3_cord_3Dgl_collect,
  Vertex_rel_3Dgl_collect,

  /* Swap Screen */
  SwapScreen_3Dgl_collect,

  /* Get Number of polygons */
  get_num_polygons_3Dgl_collect,

  /* Get number of vertices */
  get_num_vertices_3Dgl_collect,

  /* Flush */
  Flush_3Dgl_collect,

  /* poly attribute */
  PolyAttr_3Dgl_collect,

  /* Material 0 */
  material0_3Dgl_collect,

  /* Material 1 */
  material1_3Dgl_collect,

  /* Shininess */
  shininess_3Dgl_collect,

  /* Texture attributes */
  TextImage_3Dgl_collect,

  /* Texture palette */
  texture_palette_3Dgl_collect,

  /* Texture coordinate */
  texture_coord_3Dgl_collect,

  /* Light direction */
  light_direction_3Dgl_collect,

  /* Light colour */
  light_colour_3Dgl_collect,

  /* Alpha function */
  alpha_function_3Dgl_collect,

  /* Control */
  control_3Dgl_collect,

  /* normal */
  normal_3Dgl_collect,

  /* Call list */
  call_list_3Dgl_collect,

  /* Get clip matrix */
  get_clip_matrix_3Dgl_collect,

  /* Get direction matrix */
  get_direction_matrix_3Dgl_collect,

  /* get line */
  get_line_3Dgl_collect
};



#endif /* End of __MINGW32__ */
