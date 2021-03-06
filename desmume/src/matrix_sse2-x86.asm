;
;	Copyright (C) 2006 yopyop
;	Copyright (C) 2008 CrazyMax
;
;    This file is part of DeSmuME
;
;    DeSmuME is free software; you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation; either version 2 of the License, or
;    (at your option) any later version.
;
;    DeSmuME is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with DeSmuME; if not, write to the Free Software
;    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	TITLE	matrix_sse2-x86.asm
	.686P
	.XMM
	.model	flat
	.code
	
@MatrixMultVec4x4@8 PROC PUBLIC
		movaps	xmm0, XMMWORD PTR [ecx]
		movaps	xmm1, XMMWORD PTR [ecx+16]
		movaps	xmm2, XMMWORD PTR [ecx+32]
		movaps	xmm3, XMMWORD PTR [ecx+48]
		movaps	xmm4, XMMWORD PTR [edx]
		movaps	xmm5, xmm4
		movaps	xmm6, xmm4
		movaps	xmm7, xmm4
		shufps	xmm4, xmm4, 00000000b
		shufps	xmm5, xmm5, 01010101b
		shufps	xmm6, xmm6, 10101010b
		shufps	xmm7, xmm7, 11111111b
		mulps	xmm4, xmm0
		mulps	xmm5, xmm1
		mulps	xmm6, xmm2
		mulps	xmm7, xmm3
		addps	xmm4, xmm5
		addps	xmm4, xmm6
		addps	xmm4, xmm7
		movaps	XMMWORD PTR [edx], xmm4
		ret		0
@MatrixMultVec4x4@8 ENDP

@MatrixMultVec3x3@8 PROC PUBLIC
		movaps	xmm0, XMMWORD PTR [ecx]
		movaps	xmm1, XMMWORD PTR [ecx+16]
		movaps	xmm2, XMMWORD PTR [ecx+32]
		movaps	xmm4, XMMWORD PTR [edx]
		movaps	xmm5, xmm4
		movaps	xmm6, xmm4
		movaps	xmm7, xmm4
		shufps	xmm4, xmm4, 00000000b
		shufps	xmm5, xmm5, 01010101b
		shufps	xmm6, xmm6, 10101010b
		mulps	xmm4, xmm0
		mulps	xmm5, xmm1
		mulps	xmm6, xmm2
		addps	xmm4, xmm5
		addps	xmm4, xmm6
		movaps	XMMWORD PTR [edx], xmm4
		ret		0
@MatrixMultVec3x3@8 ENDP

@MatrixMultiply@8 PROC PUBLIC
		movaps	xmm0, XMMWORD PTR [ecx]
		movaps	xmm1, XMMWORD PTR [ecx+16]
		movaps	xmm2, XMMWORD PTR [ecx+32]
		movaps	xmm3, XMMWORD PTR [ecx+48]
		movaps	xmm4, XMMWORD PTR [edx]			; r00, r01, r02, r03
		movaps	xmm5,xmm4
		movaps	xmm6,xmm4
		movaps	xmm7,xmm4
		shufps	xmm4,xmm4,00000000b
		shufps	xmm5,xmm5,01010101b
		shufps	xmm6,xmm6,10101010b
		shufps	xmm7,xmm7,11111111b
		mulps	xmm4,xmm0
		mulps	xmm5,xmm1
		mulps	xmm6,xmm2
		mulps	xmm7,xmm3
		addps	xmm4,xmm5
		addps	xmm4,xmm6
		addps	xmm4,xmm7
		movaps	XMMWORD PTR [ecx],xmm4
		movaps	xmm4, XMMWORD PTR [edx+16]		; r04, r05, r06, r07
		movaps	xmm5,xmm4
		movaps	xmm6,xmm4
		movaps	xmm7,xmm4
		shufps	xmm4,xmm4,00000000b
		shufps	xmm5,xmm5,01010101b
		shufps	xmm6,xmm6,10101010b
		shufps	xmm7,xmm7,11111111b
		mulps	xmm4,xmm0
		mulps	xmm5,xmm1
		mulps	xmm6,xmm2
		mulps	xmm7,xmm3
		addps	xmm4,xmm5
		addps	xmm4,xmm6
		addps	xmm4,xmm7
		movaps	XMMWORD PTR [ecx+16],xmm4
		movaps	xmm4, XMMWORD PTR [edx+32]		; r08, r09, r10, r11
		movaps	xmm5,xmm4
		movaps	xmm6,xmm4
		movaps	xmm7,xmm4
		shufps	xmm4,xmm4,00000000b
		shufps	xmm5,xmm5,01010101b
		shufps	xmm6,xmm6,10101010b
		shufps	xmm7,xmm7,11111111b
		mulps	xmm4,xmm0
		mulps	xmm5,xmm1
		mulps	xmm6,xmm2
		mulps	xmm7,xmm3
		addps	xmm4,xmm5
		addps	xmm4,xmm6
		addps	xmm4,xmm7
		movaps	XMMWORD PTR [ecx+32],xmm4
		movaps	xmm4, XMMWORD PTR [edx+48]		; r12, r13, r14, r15
		movaps	xmm5,xmm4
		movaps	xmm6,xmm4
		movaps	xmm7,xmm4
		shufps	xmm4,xmm4,00000000b
		shufps	xmm5,xmm5,01010101b
		shufps	xmm6,xmm6,10101010b
		shufps	xmm7,xmm7,11111111b
		mulps	xmm4,xmm0
		mulps	xmm5,xmm1
		mulps	xmm6,xmm2
		mulps	xmm7,xmm3
		addps	xmm4,xmm5
		addps	xmm4,xmm6
		addps	xmm4,xmm7
		movaps	XMMWORD PTR [ecx+48],xmm4
		ret		0
@MatrixMultiply@8 ENDP

@MatrixTranslate@8 PROC PUBLIC
		movaps	xmm0, XMMWORD PTR [ecx]
		movaps	xmm1, XMMWORD PTR [ecx+16]
		movaps	xmm2, XMMWORD PTR [ecx+32]
		movaps	xmm3, XMMWORD PTR [ecx+48]
		movaps	xmm4, XMMWORD PTR [edx]
		movaps	xmm5, xmm4
		movaps	xmm6, xmm4
		movaps	xmm7, xmm4
		shufps	xmm4, xmm4, 00000000b
		shufps	xmm5, xmm5, 01010101b
		shufps	xmm6, xmm6, 10101010b
		mulps	xmm4, xmm0
		mulps	xmm5, xmm1
		mulps	xmm6, xmm2
		addps	xmm4, xmm5
		addps	xmm4, xmm6
		addps	xmm4, xmm3
		movaps	XMMWORD PTR [ecx+48], xmm4
		ret		0
@MatrixTranslate@8 ENDP

@MatrixScale@8 PROC PUBLIC
		movaps	xmm0, XMMWORD PTR [ecx]
		movaps	xmm1, XMMWORD PTR [ecx+16]
		movaps	xmm2, XMMWORD PTR [ecx+32]
		movaps	xmm4, XMMWORD PTR [edx]
		movaps	xmm5, xmm4
		movaps	xmm6, xmm4
		shufps	xmm4, xmm4, 00000000b
		shufps	xmm5, xmm5, 01010101b
		shufps	xmm6, xmm6, 10101010b
		mulps	xmm4, xmm0
		mulps	xmm5, xmm1
		mulps	xmm6, xmm2
		movaps	XMMWORD PTR [ecx],xmm4
		movaps	XMMWORD PTR [ecx+16],xmm5
		movaps	XMMWORD PTR [ecx+32],xmm6
		ret		0
@MatrixScale@8 ENDP

end

