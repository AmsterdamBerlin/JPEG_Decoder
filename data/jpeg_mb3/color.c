/*-----------------------------------------*/
/* File : color.c, utilities for jfif view */
/* Author : Pierre Guerrier, march 1998	   */
/*-----------------------------------------*/

//#include <stdlib.h>
//#include <stdio.h>
#include <global_memmap.h>
#include "jpeg.h"
#include <stdint.h>


/* Ensure number is >=0 and <=255			   */
#define Saturate(n)	((n) > 0 ? ((n) < 255 ? (n) : 255) : 0)

/*---------------------------------------*/
/* rules for color conversion:	         */
/*  r = y		+1.402	v	 */
/*  g = y -0.34414u	-0.71414v	 */
/*  b = y +1.772  u			 */
/* Approximations: 1.402 # 7/5 = 1.400	 */
/*		.71414 # 357/500 = 0.714 */
/*		.34414 # 43/125	= 0.344	 */
/*		1.772  = 443/250	 */
/*---------------------------------------*/
/* Approximations: 1.402 # 359/256 = 1.40234 */
/*		.71414 # 183/256 = 0.71484 */
/*		.34414 # 11/32 = 0.34375 */
/*		1.772 # 227/128 = 1.7734 */
/*----------------------------------*/

typedef union{
	struct {
		uint8_t B,G,R,A;
	};
	uint32_t value;
} FB;





void black_conversion(void)
{
	int i, j;
	unsigned char y;
	long r, g, b;
	long offset;

	for (i = 0; i < MCU_sy; i++)
		{
			int ip_0 = i >> comp[0].VDIV;
			int ip_0_lsbs = ip_0 & 7;
			int inv_ndx_0 = comp[0].IDX + comp[0].HS * (ip_0 >> 3);
			int i_times_MCU_sx = i * MCU_sx;
			for (j = 0; j < MCU_sx; j++)
			{
				int jp_0 = j >> comp[0].HDIV;
				y = MCU_buff[inv_ndx_0 + (jp_0 >> 3)]->block[ip_0_lsbs][jp_0 & 7];
				r = y;
				g = y;
				b = y;
				offset = i_times_MCU_sx + j;
				FB f;
				f.R = Saturate(r);
				f.G = Saturate(g);
				f.B = Saturate(b);
				ColorBuffer[offset] = f.value;
				//fb[1024*(i+MCU_row*MCU_sy)+j+MCU_column*MCU_sx] = f.value;
			}
		}
}

void color_conversion(void)
{
	int i, j;
	unsigned char y, cb, cr;
	signed char rcb, rcr;
	long r, g, b;
	long offset;

	for (i = 0; i < MCU_sy; i++) {	/* pixel rows */
		int ip_0 = i >> comp[0].VDIV;
		int ip_1 = i >> comp[1].VDIV;
		int ip_2 = i >> comp[2].VDIV;
		int inv_ndx_0 = comp[0].IDX + comp[0].HS * (ip_0 >> 3);
		int inv_ndx_1 = comp[1].IDX + comp[1].HS * (ip_1 >> 3);
		int inv_ndx_2 = comp[2].IDX + comp[2].HS * (ip_2 >> 3);
		int ip_0_lsbs = ip_0 & 7;
		int ip_1_lsbs = ip_1 & 7;
		int ip_2_lsbs = ip_2 & 7;
		int i_times_MCU_sx = i * MCU_sx;

		for (j = 0; j < MCU_sx; j++) {	/* pixel columns */
			int jp_0 = j >> comp[0].HDIV;
			int jp_1 = j >> comp[1].HDIV;
			int jp_2 = j >> comp[2].HDIV;

			y = MCU_buff[inv_ndx_0 + (jp_0 >> 3)]->block[ip_0_lsbs][jp_0 & 7];
			cb = MCU_buff[inv_ndx_1 + (jp_1 >> 3)]->block[ip_1_lsbs][jp_1 & 7];
			cr = MCU_buff[inv_ndx_2 + (jp_2 >> 3)]->block[ip_2_lsbs][jp_2 & 7];

			rcb = cb - 128;
			rcr = cr - 128;

			r = y + ((359 * rcr) >> 8);
			g = y - ((11 * rcb) >> 5) - ((183 * rcr) >> 8);
			b = y + ((227 * rcb) >> 7);

			offset = i_times_MCU_sx + j;
			FB f;
			f.R = Saturate(r);
			f.G = Saturate(g);
			f.B = Saturate(b);
			
			// buffer color 
			ColorBuffer[offset] = f.value;
			//hw_dma_send(fb + offset , &ColorBuffer[i * MCU_sx], goodcolumns, &dma, NON_BLOCKING);
			
			//	fb[1024*(i+MCU_row*MCU_sy)+j+MCU_column*MCU_sx] = f.value;
			//mk_mon_debug_info(offset);
			//hw_dma_send(&FrameBuffer[1024*(i+MCU_row*MCU_sy)+j+MCU_column*MCU_sx] , &ColorBuffer[offset], 1, &dma, BLOCKING);
			/*
			if ( c_count < 256){
				
				//Big_fb[1024*(i+MCU_row*MCU_sy)+j+MCU_column*MCU_sx] = f.value;
			//	fb[1024*(i+MCU_row*MCU_sy)+j+MCU_column*MCU_sx] = f.value;
			}
			else {
			 	c_count = 0;
			 	hw_dma_send(&fb[k++*1024], Big_fb, 1024, &dma1, NON_BLOCKING);
			} */

		}
	}
}
