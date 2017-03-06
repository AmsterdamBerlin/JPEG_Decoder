/*-----------------------------------------*/
/* File : utils.c, utilities for jfif view */
/* Author : Pierre Guerrier, march 1998	   */
/*-----------------------------------------*/

//#include <stdlib.h>
//#include <stdio.h>
#include <comik.h>

#include "jpeg.h"

/*----------------------------------------------------------*/

/* Returns ceil(N/D). */
int ceil_div(int N, int D)
{
	int i = N / D;

	if (N > D * i)
		i++;
	return i;
}

/* Returns floor(N/D). */
int floor_div(int N, int D)
{
	int i = N / D;

	if (N < D * i)
		i--;
	return i;
}

/*----------------------------------------------------------*/

/* For all components reset DC prediction value to 0. */
void reset_prediction(void)
{
	int i;

	for (i = 0; i < 3; i++)
		comp[i].PRED = 0;
}

/*---------------------------------------------------------*/

/* Transform JPEG number format into usual 2's-complement format. */
int reformat(unsigned long S, int good)
{
	int St;

	if (!good)
		return 0;
	St = 1 << (good - 1);	/* 2^(good-1) */
	if (S < (unsigned long)St)
		return (S + 1 + ((-1) << good));
	else
		return S;
}

/*----------------------------------------------------------*/

void free_structures(void)
{
	int i;

	for (i = 0; i < 4; i++)
		if (QTvalid[i])
			mk_free(QTable[i]);

	/* if (ColorBuffer != NULL)
		mk_free(ColorBuffer); */
	/* if (FrameBuffer != NULL)
		mk_free(FrameBuffer); */
	if (PBuff != NULL)
		mk_free(PBuff);
	if (FBuff != NULL)
		mk_free(FBuff);

	for (i = 0; MCU_valid[i] != -1; i++)
		mk_free(MCU_buff[i]);
}
