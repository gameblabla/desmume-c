#ifdef GKD350H

#include <stdint.h>
#include <stdlib.h>

//from BGR555
#define cR(A) (((A) & 0x001f) << 3)
#define cG(A) (((A) & 0x03e0) >> 2)
#define cB(A) (((A) & 0x7c00) >> 7)
//to BGR555
#define Weight1_1(A, B)  ((((cB(A) + cB(B)) >> 1) & 0xf8) << 7 | (((cG(A) + cG(B)) >> 1) & 0xf8) << 2 | (((cR(A) + cR(B)) >> 1) & 0xf8) >> 3)
#define Weight5_3(A, B)  (((((cB(A) * 5) + (cB(B) * 3)) >> 3) & 0xf8) << 7 | ((((cG(A) * 5) + (cG(B) * 3)) >> 3) & 0xf8) << 2 | ((((cR(A) * 5) + (cR(B) * 3)) >> 3) & 0xf8) >> 3)
#define Weight3_5(A, B)  (((((cB(A) * 3) + (cB(B) * 5)) >> 3) & 0xf8) << 7 | ((((cG(A) * 3) + (cG(B) * 5)) >> 3) & 0xf8) << 2 | ((((cR(A) * 3) + (cR(B) * 5)) >> 3) & 0xf8) >> 3)
#define Weight2_6(A, B)  (((((cB(A) * 2) + (cB(B) * 6)) >> 3) & 0xf8) << 7 | ((((cG(A) * 2) + (cG(B) * 6)) >> 3) & 0xf8) << 2 | ((((cR(A) * 2) + (cR(B) * 6)) >> 3) & 0xf8) >> 3)
#define Weight6_2(A, B)  (((((cB(A) * 6) + (cB(B) * 2)) >> 3) & 0xf8) << 7 | ((((cG(A) * 6) + (cG(B) * 2)) >> 3) & 0xf8) << 2 | ((((cR(A) * 6) + (cR(B) * 2)) >> 3) & 0xf8) >> 3)
//to RGB565
#define WeightB1_1(A, B)  ((((cR(A) + cR(B)) >> 1) & 0xf8) << 8 | (((cG(A) + cG(B)) >> 1) & 0xfc) << 3 | (((cB(A) + cB(B)) >> 1) & 0xf8) >> 3)
#define WeightB5_3(A, B)  (((((cR(A) * 5) + (cR(B) * 3)) >> 3) & 0xf8) << 8 | ((((cG(A) * 5) + (cG(B) * 3)) >> 3) & 0xfc) << 3 | ((((cB(A) * 5) + (cB(B) * 3)) >> 3) & 0xf8) >> 3)
#define WeightB3_5(A, B)  (((((cR(A) * 3) + (cR(B) * 5)) >> 3) & 0xf8) << 8 | ((((cG(A) * 3) + (cG(B) * 5)) >> 3) & 0xfc) << 3 | ((((cB(A) * 3) + (cB(B) * 5)) >> 3) & 0xf8) >> 3)
#define WeightB2_6(A, B)  (((((cR(A) * 2) + (cR(B) * 6)) >> 3) & 0xf8) << 8 | ((((cG(A) * 2) + (cG(B) * 6)) >> 3) & 0xfc) << 3 | ((((cB(A) * 2) + (cB(B) * 6)) >> 3) & 0xf8) >> 3)
#define WeightB6_2(A, B)  (((((cR(A) * 6) + (cR(B) * 2)) >> 3) & 0xf8) << 8 | ((((cG(A) * 6) + (cG(B) * 2)) >> 3) & 0xfc) << 3 | ((((cB(A) * 6) + (cB(B) * 2)) >> 3) & 0xf8) >> 3)
 
void scale_256x384_to_160x240(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
 
    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    uint16_t a1, a2, a3, a4, a5, a6, a7, a8, b1, b2, b3, b4, b5, b6, b7, b8;
    for (BlockY = 0; BlockY < 48; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 256 * 8;
        BlockDst = Dst16 + 80 + BlockY * 320 * 5; // 80 is the offset for centering the image into the 320-width destination surface
        for (BlockX = 0; BlockX < 32; BlockX++)
        {
            // 8x8 to 5x5
            // Before: (a)(b)(c)(d)(e)(f)(g)(h)  After: (aaaaabbb)(bbcccccc)(de)(ffffffgg)(ggghhhhh)
 
            // -- Row 1 --;
            a1 = *(BlockSrc               );
            a2 = *(BlockSrc            + 1);
            b1 = *(BlockSrc + 256 *  1    );
            b2 = *(BlockSrc + 256 *  1 + 1);
            *(BlockDst              ) = WeightB5_3(Weight5_3( a1, a2), Weight5_3( b1, b2));
            a3 = *(BlockSrc            + 2);
            b3 = *(BlockSrc + 256 *  1 + 2);
            *(BlockDst           + 1) = WeightB5_3(Weight2_6( a2, a3), Weight2_6( b2, b3));
            a4 = *(BlockSrc            + 3);
            a5 = *(BlockSrc            + 4);
            b4 = *(BlockSrc + 256 *  1 + 3);
            b5 = *(BlockSrc + 256 *  1 + 4);
            *(BlockDst           + 2) = WeightB5_3(Weight1_1( a4, a5), Weight1_1( b4, b5));
            a6 = *(BlockSrc            + 5);
            a7 = *(BlockSrc            + 6);
            b6 = *(BlockSrc + 256 *  1 + 5);
            b7 = *(BlockSrc + 256 *  1 + 6);
            *(BlockDst           + 3) = WeightB5_3(Weight6_2( a6, a7), Weight6_2( b6, b7));
            a8 = *(BlockSrc            + 7);
            b8 = *(BlockSrc + 256 *  1 + 7);
            *(BlockDst           + 4) = WeightB5_3(Weight3_5( a7, a8), Weight3_5( b7, b8));
 
            // -- Row 2 --
            a1 = *(BlockSrc + 256 *  2    );
            a2 = *(BlockSrc + 256 *  2 + 1);
            *(BlockDst + 320 * 1    ) = WeightB2_6(Weight5_3( b1, b2), Weight5_3( a1, a2));
            a3 = *(BlockSrc + 256 *  2 + 2);
            a4 = *(BlockSrc + 256 *  2 + 3);
            *(BlockDst + 320 * 1 + 1) = WeightB2_6(Weight2_6( b2, b3), Weight2_6( a2, a3));
            a5 = *(BlockSrc + 256 *  2 + 4);
            *(BlockDst + 320 * 1 + 2) = WeightB2_6(Weight1_1( b4, b5), Weight1_1( a4, a5));
            a6 = *(BlockSrc + 256 *  2 + 5);
            a7 = *(BlockSrc + 256 *  2 + 6);
            *(BlockDst + 320 * 1 + 3) = WeightB2_6(Weight6_2( b6, b7), Weight6_2( a6, a7));
            a8 = *(BlockSrc + 256 *  2 + 7);
            *(BlockDst + 320 * 1 + 4) = WeightB2_6(Weight3_5( b7, b8), Weight3_5( a7, a8));
 
            // -- Row 3 --
            a1 = *(BlockSrc + 256 *  3    );
            a2 = *(BlockSrc + 256 *  3 + 1);
            b1 = *(BlockSrc + 256 *  4    );
            b2 = *(BlockSrc + 256 *  4 + 1);
            *(BlockDst + 320 * 2    ) = WeightB1_1(Weight5_3( a1, a2), Weight5_3( b1, b2));
            a3 = *(BlockSrc + 256 *  3 + 2);
            b3 = *(BlockSrc + 256 *  4 + 2);
            *(BlockDst + 320 * 2 + 1) = WeightB1_1(Weight2_6( a2, a3), Weight2_6( b2, b3));
            a4 = *(BlockSrc + 256 *  3 + 3);
            a5 = *(BlockSrc + 256 *  3 + 4);
            b4 = *(BlockSrc + 256 *  4 + 3);
            b5 = *(BlockSrc + 256 *  4 + 4);
            *(BlockDst + 320 * 2 + 2) = WeightB1_1(Weight1_1( a4, a5), Weight1_1( b4, b5));
            a6 = *(BlockSrc + 256 *  3 + 5);
            a7 = *(BlockSrc + 256 *  3 + 6);
            b6 = *(BlockSrc + 256 *  4 + 5);
            b7 = *(BlockSrc + 256 *  4 + 6);
            *(BlockDst + 320 * 2 + 3) = WeightB1_1(Weight6_2( a6, a7), Weight6_2( b6, b7));
            a8 = *(BlockSrc + 256 *  3 + 7);
            b8 = *(BlockSrc + 256 *  4 + 7);
            *(BlockDst + 320 * 2 + 4) = WeightB1_1(Weight3_5( a7, a8), Weight3_5( b7, b8));
 
            // -- Row 4 --
            a1 = *(BlockSrc + 256 *  5    );
            a2 = *(BlockSrc + 256 *  5 + 1);
            b1 = *(BlockSrc + 256 *  6    );
            b2 = *(BlockSrc + 256 *  6 + 1);
            *(BlockDst + 320 * 3    ) = WeightB6_2(Weight5_3( a1, a2), Weight5_3( b1, b2));
            a3 = *(BlockSrc + 256 *  5 + 2);
            b3 = *(BlockSrc + 256 *  6 + 2);
            *(BlockDst + 320 * 3 + 1) = WeightB6_2(Weight2_6( a2, a3), Weight2_6( b2, b3));
            a4 = *(BlockSrc + 256 *  5 + 3);
            a5 = *(BlockSrc + 256 *  5 + 4);
            b4 = *(BlockSrc + 256 *  6 + 3);
            b5 = *(BlockSrc + 256 *  6 + 4);
            *(BlockDst + 320 * 3 + 2) = WeightB6_2(Weight1_1( a4, a5), Weight1_1( b4, b5));
            a6 = *(BlockSrc + 256 *  5 + 5);
            a7 = *(BlockSrc + 256 *  5 + 6);
            b6 = *(BlockSrc + 256 *  6 + 5);
            b7 = *(BlockSrc + 256 *  6 + 6);
            *(BlockDst + 320 * 3 + 3) = WeightB6_2(Weight6_2( a6, a7), Weight6_2( b6, b7));
            a8 = *(BlockSrc + 256 *  5 + 7);
            b8 = *(BlockSrc + 256 *  6 + 7);
            *(BlockDst + 320 * 3 + 4) = WeightB6_2(Weight3_5( a7, a8), Weight3_5( b7, b8));
 
            // -- Row 5 --
            a1 = *(BlockSrc + 256 *  7    );
            a2 = *(BlockSrc + 256 *  7 + 1);
            *(BlockDst + 320 * 4    ) = WeightB3_5(Weight5_3( b1, b2), Weight5_3( a1, a2));
            a3 = *(BlockSrc + 256 *  7 + 2);
            *(BlockDst + 320 * 4 + 1) = WeightB3_5(Weight2_6( b2, b3), Weight2_6( a2, a3));
            a4 = *(BlockSrc + 256 *  7 + 3);
            a5 = *(BlockSrc + 256 *  7 + 4);
            *(BlockDst + 320 * 4 + 2) = WeightB3_5(Weight1_1( b4, b5), Weight1_1( a4, a5));
            a6 = *(BlockSrc + 256 *  7 + 5);
            a7 = *(BlockSrc + 256 *  7 + 6);
            *(BlockDst + 320 * 4 + 3) = WeightB3_5(Weight6_2( b6, b7), Weight6_2( a6, a7));
            a8 = *(BlockSrc + 256 *  7 + 7);
            *(BlockDst + 320 * 4 + 4) = WeightB3_5(Weight3_5( b7, b8), Weight3_5( a7, a8));
 
            BlockSrc += 8;
            BlockDst += 5;
        }
    }
}
#endif
