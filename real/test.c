#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#define PUREC
#include "assembly.h"


	int32_t x=64000;
	int32_t y=-640000;
	int64_t sum;
void main()
{
int i;
	srand(time(NULL));
	for (i = 1; i < 10; i++)
	{
		x = rand()*640000;
		y = rand()%16;
		sum = rand()*640000;
//		printf("x=%li (%lx); y=%li (%lx); etalon= %li\n",x,x,y,y,MULSHIFT32(x,y));
//		printf("x=%li (%lx); y=%li (%lx); testas= %li\n",x,x,y,y,MULSHIFT32A(x,y));
		printf("x=%li (%lx); y=%li (%lx); etalon= %li\n",x,x,y,y,SAR64(x,y));
		printf("x=%li (%lx); y=%li (%lx); testas= %li\n",x,x,y,y,SAR64A(x,y));
	}
}
