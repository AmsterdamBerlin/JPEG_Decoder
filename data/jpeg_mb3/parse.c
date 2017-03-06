/*-----------------------------------------*/
/* File : parse.c, utilities for jfif view */
/* Author : Pierre Guerrier, march 1998	   */
/*-----------------------------------------*/

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
#include <comik.h>
#include "jpeg.h"
#include <hw_dma.h>
#include "timers.h"


/*---------------------------------------------------------------------*/

/* utility and counter to return the number of bits from file */
/* right aligned, masked, first bit towards MSB's		*/

static unsigned char bit_count;	/* available bits in the window */
static unsigned char window;

//unsigned long get_bits(FILE * fi, int number)
unsigned long get_bits(int number)
{
	int i, newbit;
	unsigned long result = 0;
	unsigned char aux, wwindow;

	if (!number)
		return 0;

	for (i = 0; i < number; i++) {
		if (bit_count == 0) {
			wwindow = FGETC();

			if (wwindow == 0xFF)
				switch (aux = FGETC()) {	/* skip stuffer 0 byte */
				case 0xFF:
					break;

				case 0x00:
					stuffers++;
					break;

				default:
					FTELL();
					break;
				}

			bit_count = 8;
		} else
			wwindow = window;
		newbit = (wwindow >> 7) & 1;
		window = wwindow << 1;
		bit_count--;
		result = (result << 1) | newbit;
	}
	return result;
}

void clear_bits(void)
{
	bit_count = 0;
}

//unsigned char get_one_bit(FILE * fi)
unsigned char get_one_bit()
{
	int newbit;
	unsigned char aux, wwindow;

	if (bit_count == 0) {
		wwindow = FGETC();

		if (wwindow == 0xFF)
			switch (aux = FGETC()) {	/* skip stuffer 0 byte */
			case 0xFF:
				break;

			case 0x00:
				stuffers++;
				break;

			default:
				break;
			}

		bit_count = 8;
	} else
		wwindow = window;

	newbit = (wwindow >> 7) & 1;
	window = wwindow << 1;
	bit_count--;
	return newbit;
}

/*----------------------------------------------------------*/

//unsigned int get_size(FILE * fi)
unsigned int get_size()
{
	unsigned char aux;

	aux = FGETC();
	return (aux << 8) | FGETC();	/* big endian */
}

/*----------------------------------------------------------*/

//void skip_segment(FILE * fi)
void skip_segment()
{				/* skip a segment we don't want */
	unsigned int size;
	int i;

	size = get_size();
	if (size > 5) {
		for (i = 0; i < 4; i++)
			 FGETC();
		size -= 4;
	}
	FSEEK(size - 2, 0);
}

/*----------------------------------------------------------------*/
/* find next marker of any type, returns it, positions just after */
/* EOF instead of marker if end of file met while searching ...	  */
/*----------------------------------------------------------------*/

//unsigned int get_next_MK(FILE * fi)
unsigned int get_next_MK()
{
	unsigned int c;
	int ffmet = 0;
	int locpassed = -1;
	passed--;		/* as we fetch one anyway */
	while ((c = FGETC()) != (unsigned int)EOF) {
		switch (c) {
		case 0xFF:
			ffmet = 1;
			break;
		case 0x00:
			ffmet = 0;
			break;
		default:
			if (ffmet)
				//mk_mon_debug_info(101010);
				return (0xFF00 | c);
			ffmet = 0;
			break;
		}
		locpassed++;
		passed++;
	}

	return (unsigned int)EOF;
}

/*----------------------------------------------------------*/
/* loading and allocating of quantization table             */
/* table elements are in ZZ order (same as unpack output)   */
/*----------------------------------------------------------*/

//int load_quant_tables(FILE * fi)
int load_quant_tables()
{
	char aux;
	unsigned int size, n, i, id, x;

	size = get_size();	/* this is the tables' size */
	n = (size - 2) / 65;

	for (i = 0; i < n; i++) {
		aux = FGETC();
		if (first_quad(aux) > 0) {
			//fprintf(stderr, "\tERROR:\tBad QTable precision!\n");
			return -1;
		}
		id = second_quad(aux);
		QTable[id] = (PBlock *) mk_malloc(sizeof(PBlock));
		if (QTable[id] == NULL) {
			//fprintf(stderr, "\tERROR:\tCould not allocate table storage!\n");
			//exit(1);
            return 1;
		}
		QTvalid[id] = 1;
		for (x = 0; x < 64; x++)
			QTable[id]->linear[x] = FGETC();
	}
	return 0;
}

/*----------------------------------------------------------*/
/* initialise MCU block descriptors	                    */
/*----------------------------------------------------------*/

int init_MCU(void)
{
	int i, j, k, n, hmax = 0, vmax = 0;

	for (i = 0; i < 10; i++)
		MCU_valid[i] = -1;

	k = 0;

	for (i = 0; i < n_comp; i++) {
		if (comp[i].HS > hmax)
			hmax = comp[i].HS;
		if (comp[i].VS > vmax)
			vmax = comp[i].VS;
		n = comp[i].HS * comp[i].VS;

		comp[i].IDX = k;
		for (j = 0; j < n; j++) {
			MCU_valid[k] = i;
			MCU_buff[k] = (PBlock *) mk_malloc(sizeof(PBlock));
			if (MCU_buff[k] == NULL) {
                return 1;
			}
			k++;
			if (k == 10) {
				return -1;
			}
		}
	}

	MCU_sx = 8 * hmax;
	MCU_sy = 8 * vmax;
	for (i = 0; i < n_comp; i++) {
		comp[i].HDIV = (hmax / comp[i].HS > 1);	/* if 1 shift by 0 */
		comp[i].VDIV = (vmax / comp[i].VS > 1);	/* if 2 shift by one */
	}

	mx_size = ceil_div(x_size, MCU_sx);
	my_size = ceil_div(y_size, MCU_sy);
	rx_size = MCU_sx * floor_div(x_size, MCU_sx);
	ry_size = MCU_sy * floor_div(y_size, MCU_sy);

	return 0;
}

/*----------------------------------------------------------*/
/* this takes care for processing all the blocks in one MCU */
/*----------------------------------------------------------*/

//volatile unsigned int *fb = (unsigned int *)shared_pt_REMOTEADDR;
//volatile unsigned int *ColorBuffer = (unsigned int *)mb1_cmemout0_BASEADDR; 


//int process_MCU(FILE * fi)
int process_MCU()
{
	int i;
	long offset;
	int goodrows, goodcolumns;
	
	if (MCU_column == mx_size) {
		MCU_column = 0;
		MCU_row++;
		if (MCU_row == my_size) {
			in_frame = 0;
			return 0;
		}
		if (verbose)
            return 1; 
	}

	for (curcomp = 0; MCU_valid[curcomp] != -1; curcomp++) {
		timer_start(0);
		unpack_block(FBuff, MCU_valid[curcomp]);	/* pass index to HT,QT,pred */
		timer_stop(0);
		if(MCU_count%4==2) {
			timer_start(1);
			IDCT(FBuff, MCU_buff[curcomp]);
			timer_stop(1);
		}
	}

	/* YCrCb to RGB color space transform here */
	if(MCU_count%4==2)
	{
		if (n_comp > 1){
			timer_start(2);
			color_conversion();
			timer_stop(2);
			c_count++;
		}
		else {
			timer_start(3);
			black_conversion();
			timer_stop(3);
		}
	}
	
	/* cut last row/column as needed in pixels */
	if ((y_size != ry_size) && (MCU_row == (my_size - 1)))
		goodrows = y_size - ry_size; /* down-rounded Video frame size in pixel units, multiple of MCU */
	else
		goodrows = MCU_sy;

	if ((x_size != rx_size) && (MCU_column == (mx_size - 1)))
		goodcolumns = x_size - rx_size;
	else
		goodcolumns = MCU_sx; //  MCU size in pixels 

	
	offset = (MCU_row * MCU_sy ) * 1024 + MCU_column * MCU_sx;  // 
	if(MCU_count%4==2)
	{
		for (i = 0; i < goodrows; i++){
			hw_dma_send(FrameBuffer + offset + 1024*i, &ColorBuffer[i * MCU_sx], goodcolumns, &dma, NON_BLOCKING);
			while(hw_dma_status(&dma));
		}
	}
	
	MCU_column++;
	return 1;
}
