/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder
 * Jon Recker (jrecker@real.com), Ken Cooke (kenc@real.com)
 * June 2003
 *
 * assembly.h - assembly language functions and prototypes for supported platforms
 *
 * - inline rountines with access to 64-bit multiply results 
 * - x86 (_WIN32) and ARM (ARM_ADS, _WIN32_WCE) versions included
 * - some inline functions are mix of asm and C for speed
 * - some functions are in native asm files, so only the prototype is given here
 *
 * MULSHIFT32(x, y)    signed multiply of two 32-bit integers (x and y), returns top 32 bits of 64-bit result
 * FASTABS(x)          branchless absolute value of signed integer x
 * CLZ(x)              count leading zeros in x
 * MADD64(sum, x, y)   (Windows only) sum [64-bit] += x [32-bit] * y [32-bit]
 * SHL64(sum, x, y)    (Windows only) 64-bit left shift using __int64
 * SAR64(sum, x, y)    (Windows only) 64-bit right shift using __int64
 */

#ifndef _ASSEMBLY_H
#define _ASSEMBLY_H

#if (defined _WIN32 && !defined _WIN32_WCE) || (defined __WINS__ && defined _SYMBIAN) || defined(_OPENWAVE_SIMULATOR) || defined(WINCE_EMULATOR)    /* Symbian emulator for Ix86 */

#pragma warning( disable : 4035 )	/* complains about inline asm not returning a value */

static __inline int MULSHIFT32(int x, int y)	
{
	__asm {
		mov		eax, x
		imul	y
		mov		eax, edx
	}
}

static __inline int FASTABS(int x) 
{
	int sign;

	sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;

	return x;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

/* MADD64, SHL64, SAR64:
 * write in assembly to avoid dependency on run-time lib for 64-bit shifts, muls
 *  (sometimes compiler thunks to function calls instead of code generating)
 * required for Symbian emulator
 */
#ifdef __CW32__
typedef long long Word64;
#else
typedef __int64 Word64;
#endif

static __inline Word64 MADD64(Word64 sum, int x, int y)
{
	unsigned int sumLo = ((unsigned int *)&sum)[0];
	int sumHi = ((int *)&sum)[1];

	__asm {
		mov		eax, x
		imul	y
		add		eax, sumLo
		adc		edx, sumHi
	}

	/* equivalent to return (sum + ((__int64)x * y)); */
}

static __inline Word64 SHL64(Word64 x, int n)
{
	unsigned int xLo = ((unsigned int *)&x)[0];
	int xHi = ((int *)&x)[1];
	unsigned char nb = (unsigned char)n;

	if (n < 32) {
		__asm {
			mov		edx, xHi
			mov		eax, xLo
			mov		cl, nb
			shld    edx, eax, cl
			shl     eax, cl
		}
	} else if (n < 64) {
		/* shl masks cl to 0x1f */
		__asm {
			mov		edx, xLo
			mov		cl, nb
			xor     eax, eax
			shl     edx, cl
		}
	} else {
		__asm {
			xor		edx, edx
			xor		eax, eax
		}
	}
}

static __inline Word64 SAR64(Word64 x, int n)
{
	unsigned int xLo = ((unsigned int *)&x)[0];
	int xHi = ((int *)&x)[1];
	unsigned char nb = (unsigned char)n;

	if (n < 32) {
		__asm {
			mov		edx, xHi
			mov		eax, xLo
			mov		cl, nb
			shrd	eax, edx, cl
			sar		edx, cl
		}
	} else if (n < 64) {
		/* sar masks cl to 0x1f */
		__asm {
			mov		edx, xHi
			mov		eax, xHi
			mov		cl, nb
			sar		edx, 31
			sar		eax, cl
		}
	} else {
		__asm {
			sar		xHi, 31
			mov		eax, xHi
			mov		edx, xHi
		}
	}
}

#elif (defined _WIN32) && (defined _WIN32_WCE)

/* use asm function for now (EVC++ 3.0 does horrible job compiling __int64 version) */
#define MULSHIFT32	xmp3_MULSHIFT32
int MULSHIFT32(int x, int y);

static __inline int FASTABS(int x) 
{
	int sign;

	sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;

	return x;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

#elif defined ARM_ADS

static __inline int MULSHIFT32(int x, int y)
{
	/* important rules for smull RdLo, RdHi, Rm, Rs:
	 *     RdHi and Rm can't be the same register
	 *     RdLo and Rm can't be the same register
	 *     RdHi and RdLo can't be the same register
	 * Note: Rs determines early termination (leading sign bits) so if you want to specify
	 *   which operand is Rs, put it in the SECOND argument (y)
	 * For inline assembly, x and y are not assumed to be R0, R1 so it shouldn't matter
	 *   which one is returned. (If this were a function call, returning y (R1) would 
	 *   require an extra "mov r0, r1")
	 */
	int zlow;
	__asm {
		smull zlow,y,x,y
	}

	return y;
}

static __inline int FASTABS(int x) 
{
	int t=0; /*Really is not necessary to initialiaze only to avoid warning*/

	__asm {
		eor	t, x, x, asr #31
		sub	t, t, x, asr #31
	}

	return t;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

#elif defined(__GNUC__) && defined(ARM)

#if defined(ARM7DI)
	
typedef long long Word64;

static __inline int MULSHIFT32(int x, int y) {
	return x * y;
}

static __inline Word64 SAR64(Word64 x, int n) {
	return x >>= n;
}


typedef union _U64 {
	Word64 w64;
	struct {
		/* x86 = little endian */
		unsigned int lo32;
		signed int   hi32;
	} r;
} U64;

static __inline Word64 MADD64(Word64 sum64, int x, int y)
{
	sum64 += (Word64)x * (Word64)y;

	return sum64;
}
	
#else

static __inline int MULSHIFT32(int x, int y)
{
	/* important rules for smull RdLo, RdHi, Rm, Rs:
	 *     RdHi and Rm can't be the same register
	 *     RdLo and Rm can't be the same register
	 *     RdHi and RdLo can't be the same register
	 * Note: Rs determines early termination (leading sign bits) so if you want to specify
	 *   which operand is Rs, put it in the SECOND argument (y)
	 * For inline assembly, x and y are not assumed to be R0, R1 so it shouldn't matter
	 *   which one is returned. (If this were a function call, returning y (R1) would 
	 *   require an extra "mov r0, r1")
	 */
	int zlow;
	__asm__ volatile ("smull %0,%1,%2,%3" : "=&r" (zlow), "=r" (y) : "r" (x), "1" (y)) ;

	return y;
}

#endif

static __inline int FASTABS(int x) 
{
	int t=0; /*Really is not necessary to initialiaze only to avoid warning*/

	__asm__ volatile (
		"eor %0,%2,%2, asr #31;"
		"sub %0,%1,%2, asr #31;"
		: "=&r" (t) 
		: "0" (t), "r" (x)
	 );

	return t;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

#else

#ifdef __riscv

typedef long long Word64;

static __inline int MULSHIFT32(int x, int y)
{
	unsigned int result = 0;
	asm volatile ("mulh %0, %1, %2" : "=r"(result): "r"(x), "r"(y));
	return result;
}

static __inline int FASTABS(int x) 
{
	int sign;

	sign = x >> (sizeof(int) * 8 - 1);
	x ^= sign;
	x -= sign;

	return x;
}

static __inline int CLZ(int x)
{
	int numZeros;

	if (!x)
		return (sizeof(int) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

static __inline Word64 MADD64(Word64 sum, int a, int b)
{
	unsigned int result_hi = 0;
	unsigned int result_lo = 0;
	asm volatile ("mulh %0, %1, %2" : "=r"(result_hi): "r"(a), "r"(b));
	asm volatile ("mul  %0, %1, %2" : "=r"(result_lo): "r"(a), "r"(b));

	Word64 result = result_hi;
	result <<= 32;
	result += result_lo;
	result += sum;
	return result;
}

static __inline Word64 SHL64(Word64 x, int n)
{
	return (x<<n);
}

static __inline Word64 SAR64(Word64 x, int n)
{
	return (x >> n);
}

#elif defined(PUREC)
#include <stdint.h>

/*
static __inline int32_t MULSHIFT32(int32_t x, int32_t y) {
	return (int32_t) ((((long long) x) * y ) >> 32);
}
*/

static int32_t MULSHIFT32(int32_t x, int32_t y);
#pragma aux MULSHIFT32 = \
"mul32:                                                             "\
"        xor dx,cx                        "\
"        pushf                                                      "\
"        xor dx,cx         "\
"        jns mul32_2       "\
"        not ax           "\
"        not dx                                                     "\
"        add ax,1                                                   "\
"        adc dx,0                                                   "\
"mul32_2:                                                           "\
"        test cx,cx       "\
"        jns mul32_3      "\
"        not bx           "\
"        not cx                                                     "\
"        add bx,1                                                   "\
"        adc cx,0                                                   "\
"mul32_3:                                                           "\
"                                                                   "\
"		mov si,ax	                            "\
"		mov di,dx	                          "\
"		MUL		BX                                 "\
"		;PUSH	DX                                         "\
"		PUSH	AX                                         "\
"		                                                   "\
"P2_1:		                "\
"		mov ax,si                                          "\
"		mul cx                                             "\
"		PUSH	DX                                         "\
"		PUSH	AX                                         "\
"		                                                   "\
"P2_2:		              "\
"		mov ax,di                                          "\
"		MUL		BX                                 "\
"		                                                   "\
"PART2:		POP		BX                                 "\
"		POP		si                                 "\
"		ADd		AX,	BX                         "\
"		ADC		DX,	si                         "\
"		PUSH	DX                                         "\
"		PUSH	AX                                         "\
"		                                                   "\
"PART3:		               "\
"		mov ax,di                                          "\
"		mul cx                                             "\
"		                                                   "\
"		POP		si                                 "\
"		POP		di                                 "\
"		;POP		BX                                 "\
"		POP		CX                                 "\
"		ADD		CX,	si                         "\
"		ADc		AX,	di                         "\
"		ADC		DX,	0                          "\
"		                                                   "\
"        popf                                      "\
"        jns mul32_1                    "\
"        not ax                                    "\
"        not dx                                                     "\
"mul32_1:                                                           "\
parm [ ax dx ] [ bx cx ]\
modify [ ax dx bx cx si di ];


/*
static __inline int32_t MULSHIFT32A(int32_t x, int32_t y) {
	__asm{
        //;						
        //; 32-bit signed fractional multiplication     
        //; DX:AX = (DX:AX * CX:BX) / 256               
        //;                                             
		nop				
	mov ax,word ptr x                              
	mov dx,word ptr x+2                            
	mov bx,word ptr y                              
	mov cx,word ptr y+2                            
mul32:                                                 
        xor dx,cx       ; Look for different signs     
        pushf                                          
        xor dx,cx       ; Restore DX (pair of XOR = unaffected) 
        jns mul32_2     ; If multiplicand is positive then jump. 
        not ax          ; Negate multiplicand           
        not dx                                          
        add ax,1                                        
        adc dx,0                                        
mul32_2:                                                
        test cx,cx      ; Test if multiplier is positive
        jns mul32_3     ; Is it positive? Yes, jump.    
        not bx          ; Negate multiplier             
        not cx                                          
        add bx,1                                        
        adc cx,0                                        
mul32_3:                                                
                                                        
		;MOV		AX,	WORD PTR X      
		;MOV		BX,	WORD PTR Y      
		mov si,ax	;save X                 
		mov di,dx	;save X+2               
		MUL		BX                      
		PUSH	DX                              
		PUSH	AX                              
		                                        
P2_1:		;MOV		AX,	WORD PTR x      
		mov ax,si                               
		;MOV		BX,	WORD PTR y+2    
		;MUL		BX                      
		mul cx                                  
		PUSH	DX                              
		PUSH	AX                              
		                                        
P2_2:		;MOV		AX,	WORD PTR x+2    
		mov ax,di                               
		;MOV		BX,	WORD PTR y      
		MUL		BX                      
		;mov si,cx	;save Y+2               
		                                        
PART2:		POP		BX                      
		POP		si                      
		ADd		AX,	BX              
		ADC		DX,	si              
		PUSH	DX                              
		PUSH	AX                              
		                                        
PART3:		;MOV		AX,	WORD PTR x+2    
		mov ax,di                               
		;MOV		BX,	WORD PTR y+2    
		;MUL		BX                      
		mul cx                                  
		;ADC		DX,	0               
		                                        
		POP		si                      
		POP		di                      
		POP		BX                      
		POP		CX                      
		ADD		CX,	si              
		ADc		AX,	di              
		ADC		DX,	0               
		                                        
        popf            ; Restore flags                 
        jns mul32_1     ; Different signs? No, jump.    
        not ax          ; Negate result.                
        not dx                                          
        ;add ax,1                                      
        ;adc dx,0                                      
mul32_1:                                                
	}
}
*/

static __inline int64_t SAR64(int64_t x, int32_t n) {
	return x >> n;
}

/*
static __inline int64_t MADD64(int64_t sum64, int32_t x, int32_t y)
{
	return (sum64 + ((long long)x * y));
}
*/

static inline int64_t MUL32A(int32_t x, int32_t y);
#pragma aux MUL32A = \
"mul32:                                                             "\
"        xor dx,cx                        "\
"        pushf                                                      "\
"        xor dx,cx           "\
"        jns mul32_2        "\
"        not ax                                "\
"        not dx                                                     "\
"        add ax,1                                                   "\
"        adc dx,0                                                   "\
"mul32_2:                                                           "\
"        test cx,cx                 "\
"        jns mul32_3                    "\
"        not bx                                  "\
"        not cx                                                     "\
"        add bx,1                                                   "\
"        adc cx,0                                                   "\
"mul32_3:                                                           "\
"                                                                   "\
"		mov si,ax	                             "\
"		mov di,dx	                           "\
"		MUL		BX                                  "\
"		PUSH	DX                                          "\
"		PUSH	AX                                          "\
"		                                                    "\
"P2_1:		                  "\
"		mov ax,si                                           "\
"		mul cx                                              "\
"		PUSH	DX                                          "\
"		PUSH	AX                                          "\
"		                                                    "\
"P2_2:		                "\
"		mov ax,di                                           "\
"		MUL		BX                                  "\
"		                                                    "\
"PART2:		POP		BX                                  "\
"		POP		si                                  "\
"		ADd		AX,	BX                          "\
"		ADC		DX,	si                          "\
"		PUSH	DX                                          "\
"		PUSH	AX                                          "\
"		                                                    "\
"PART3:		                "\
"		mov ax,di                                           "\
"		mul cx                                              "\
"		                                                    "\
"		POP		si                                  "\
"		POP		di                                  "\
"		POP		BX                                  "\
"		POP		CX                                  "\
"		ADD		CX,	si                          "\
"		ADc		AX,	di                          "\
"		ADC		DX,	0                           "\
"		                                                    "\
"        popf                                        "\
"        jns mul32_1                    "\
"        not ax                                    "\
"        not dx                                                     "\
"	not bx                                                      "\
"	not cx                                                      "\
"        add bx,1                                                   "\
"        adc cx,0                                                   "\
"                                                                   "\
"mul32_1:                                                           "\
"	mov di,dx"\
"	mov dx,bx"\
"	mov bx,ax"\
"	mov ax,di"\
parm [ ax dx ] [ bx cx ] \
modify [ ax dx bx cx si di ];


static __inline int64_t MADD64(int64_t sum64, int32_t x, int32_t y)
{
	return (sum64 + MUL32A(x,y));
}

static __inline int32_t FASTABS(int32_t x)
{
  return((x > 0) ? x : -(x));
}

static __inline int CLZ(int32_t x)
{
	int numZeros;

	if (!x)
		return (sizeof(int32_t) * 8);

	numZeros = 0;
	while (!(x & 0x80000000)) {
		numZeros++;
		x <<= 1;
	} 

	return numZeros;
}

//static __inline uint64_t SHL64(uint64_t x, int32_t n)
//{
//	return (x<<n);
//}


#else

#error Unsupported platform in assembly.h

#endif

#endif	/* platforms */

#endif /* _ASSEMBLY_H */
