#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#define PUREC
#include "assembly.h"


	int32_t x=64000;
	int32_t y=-640000;
	int64_t sum;

static __inline short ClipToShortA(int32_t x);
#pragma aux ClipToShortA = \
"	sar dx,1"\
"	rcr ax,1"\
\
"	sar dx,1"\
"	rcr ax,1"\
\
"	sar dx,1"\
"	rcr ax,1"\
\
"	sar dx,1"\
"	rcr ax,1"\
\
"	sar dx,1"\
"	rcr ax,1"\
\
"	sar dx,1"\
"	rcr ax,1"\
\
"	cmp     dx,-1"\
"       jz      s_ax_to_all_bits_dx"\
"       cmp     dx,0"\
"       jl      ret_minint_ax"\
"       ja      ret_maxint_ax"\
"       test    ax,ax"\
"       jns     end_clip"\
"ret_maxint_ax:"\
"       mov     ax,0x7FFF"\
"       jmp     end_clip"\
"s_ax_to_all_bits_dx:"\
"       test    dx,ax"\
"       js      end_clip"\
"ret_minint_ax:"\
"       mov     ax,0x8000"\
"end_clip:"\
parm [ ax dx ] \
modify [ ax dx ];

static __inline short ClipToShort(int32_t x, int32_t fracBits)
{
	int16_t sign;
	
	/* assumes you've already rounded (x += (1 << (fracBits-1))) */
	x >>= fracBits;
	
	/* Ken's trick: clips to [-32768, 32767] */
	sign = x >> 31;
	printf("x=%li (%lx);sign=%i\n",x,x,sign);
	if (sign != (x >> 15))
		x = sign ^ ((1ul << 15) - 1);

	return (short)x;
}

void main()
{
int i;
	srand(time(NULL));
	for (i = 1; i < 10; i++)
	{
		x = (rand() *-640l);
		y = rand()%16;
		sum = rand()*640000;
//		printf("x=%li (%lx); y=%li (%lx); etalon= %li\n",x,x,y,y,MULSHIFT32(x,y));
//		printf("x=%li (%lx); y=%li (%lx); testas= %li\n",x,x,y,y,MULSHIFT32A(x,y));
//		printf("x=%li (%lx); y=%li (%lx); etalon= %li\n",x,x,y,y,SAR64(x,y));
//		printf("x=%li (%lx); y=%li (%lx); testas= %li\n",x,x,y,y,SAR64A(x,y));
		printf("x=%li (%lx); etalon= %i\n",x,x,ClipToShort(x,6));
		printf("x=%li (%lx); testas= %i\n",x,x,ClipToShortA(x));
	}
}
