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
"mov bx, dx"\
"and bx, 0x8000"\
"neg bx"\
"sbb bx, bx"\
"mov cx, dx"\
"not cx"\
"neg cx"\
"sbb cx, cx"\
"and bx, cx"\
"and dx, cx"\
"xor bx, 0x7fff"\
"neg dx"\
"sbb dx, dx"\
"or ax, dx"\
"and dx, bx"\
"xor ax, dx"\
parm [ ax dx ] \
modify [ ax dx bx cx ];

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
