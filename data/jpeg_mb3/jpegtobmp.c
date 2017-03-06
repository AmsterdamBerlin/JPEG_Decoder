//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
#include <comik.h>

/* real declaration of global variables here */
/* see jpeg.h for more info			*/

#include "jpeg.h"

/* descriptors for 3 components */
cd_t comp[3];
/* decoded DCT blocks buffer */
PBlock *MCU_buff[10];
/* components of above MCU blocks */
int MCU_valid[10];

/* quantization tables */
PBlock *QTable[4];
int QTvalid[4];

/* Video frame size     */
int x_size, y_size;
/* down-rounded Video frame size (integer MCU) */
int rx_size, ry_size;
/* MCU size in pixels   */
int MCU_sx, MCU_sy;
/* picture size in units of MCUs */
int mx_size, my_size;
/* number of components 1,3 */
int n_comp;

/* MCU after color conversion */
volatile unsigned int *ColorBuffer;
/* complete final RGB image */
volatile unsigned int *FrameBuffer;
/* scratch frequency buffer */
FBlock *FBuff;
/* scratch pixel buffer */
PBlock *PBuff;
/* frame started ? current component ? */
int in_frame, curcomp;
/* current position in MCU unit */
int MCU_row, MCU_column;

/* input  File stream pointer   */
//FILE *fi;

/* stuff bytes in entropy coded segments */
int stuffers = 0;
/* bytes passed when searching markers */
int passed = 0;

int verbose = 0;

/* Extra global variables for 5kk03 */

int vld_count = 0;		/* Counter used by FGET and FSEEK in 5kk03.c */

/* End extra global variables for 5kk03 */

/* counter used by color buffer */
int c_count = 0;
int k = 0;
int MCU_count = 0;
/*-----------------------------------------------------------------*/
/*		MAIN		MAIN		MAIN		   */
/*-----------------------------------------------------------------*/

//int JpegToBmp(char *file1, char *file2)
int JpegToBmp()
{	
	//DMA dma = {0, mb1_dma0_BASEADDR};
	unsigned int aux, mark;
	int n_restarts, restart_interval, leftover;	/* RST check */
	int i, j;
	
	DMA_initial();
	
	/* First find the SOI marker: */
	aux = get_next_MK();
	//mk_mon_debug_info(aux);
	if (aux != SOI_MK)
        return 1;

	in_frame = 0;
	restart_interval = 0;
	for (i = 0; i < 4; i++)
		QTvalid[i] = 0;

	/* Now process segments as they appear: */
	do {
		mark = get_next_MK();
		//mk_mon_debug_info(mark);
		switch (mark) {
		case SOF_MK:
			in_frame = 1;
			get_size();	/* header size, don't care */
			//mk_mon_debug_info(1);
			/* load basic image parameters */
			FGETC();	/* precision, 8bit, don't care */
			y_size = get_size();

			x_size = get_size();


			n_comp = FGETC();	/* # of components */

			for (i = 0; i < n_comp; i++) {
				/* component specifiers */
				comp[i].CID = FGETC();
				aux = FGETC();
				comp[i].HS = first_quad(aux);
				comp[i].VS = second_quad(aux);
				comp[i].QT = FGETC();
			}

			if (init_MCU() == -1)
                return 1;

			/* dimension scan buffer for YUV->RGB conversion */
			//FrameBuffer = (unsigned char *)mk_malloc((size_t) x_size * y_size * n_comp);
			FrameBuffer = (unsigned int *)shared_pt_REMOTEADDR;
			//ColorBuffer = (unsigned char *)mk_malloc((size_t) MCU_sx * MCU_sy * n_comp);
			ColorBuffer = (unsigned int *)(mb3_cmemout0_BASEADDR + DMA_size*1*sizeof(int)); 
			
			FBuff = (FBlock *) mk_malloc(sizeof(FBlock));
			PBuff = (PBlock *) mk_malloc(sizeof(PBlock));

			if ((FrameBuffer == NULL) || (ColorBuffer == NULL) || (FBuff == NULL) || (PBuff == NULL)) {
                return 1;
			}
			break;

		case DHT_MK:
			//mk_mon_debug_info(2);
			if (load_huff_tables() == -1)
                return 1;
			break;

		case DQT_MK:
			//mk_mon_debug_info(3);
			if (load_quant_tables() == -1)
                return 1;
			break;

		case DRI_MK:
			//mk_mon_debug_info(4);
			get_size();	/* skip size */
			restart_interval = get_size();
			break;

		case SOS_MK:	/* lots of things to do here */
			//mk_mon_debug_info(5);
			//if (verbose)
				//fprintf(stderr, "%ld:\tINFO:\tFound the SOS marker!\n", FTELL(input));
			get_size();	/* don't care */
			aux = FGETC();
			if (aux != (unsigned int)n_comp) {
                return 1;
			}

			for (i = 0; i < n_comp; i++) {
				aux = FGETC();
				if (aux != comp[i].CID) {
                return 1;
				}
				aux = FGETC();
				comp[i].DC_HT = first_quad(aux);
				comp[i].AC_HT = second_quad(aux);
			}
			get_size();
			FGETC();	/* skip things */

			MCU_column = 0;
			MCU_row = 0;
			clear_bits();
			reset_prediction();
            //mk_mon_debug_info(restart_interval);
			/* main MCU processing loop here */
			if (restart_interval) {
				n_restarts = ceil_div(mx_size * my_size, restart_interval) - 1;
				leftover = mx_size * my_size - n_restarts * restart_interval;
				/* final interval may be incomplete */
				//mk_mon_debug_info(166);
				//mk_mon_debug_info(n_restarts);
				
				for (i = 0; i < n_restarts; i++) {
					for (j = 0; j < restart_interval; j++){
						process_MCU();
						MCU_count++;
					/* proc till all EOB met */
					}

					aux = get_next_MK();
					if (!RST_MK(aux)) {
                        return 1;
					}
					reset_prediction();
					clear_bits();
				}	/* intra-interval loop */
			} else
				leftover = mx_size * my_size;
			 //mk_mon_debug_info(my_size);
			 //mk_mon_debug_info(mx_size);
			/* process till end of row without restarts */
			for (i = 0; i < leftover; i++)
			{
				process_MCU();
				MCU_count++;
			}
			//mk_mon_debug_info(0xbebecafe);
			//mk_mon_debug_info(c_count);	

			in_frame = 0;

			break;

		case EOI_MK:
			//mk_mon_debug_info(6);
			if (in_frame)
				//aborted_stream(input);
                return 1;

			free_structures();
			return 0;
			break;

		case COM_MK:
			//mk_mon_debug_info(7);
			skip_segment();
			break;

		case (const int)EOF:
			//mk_mon_debug_info(8);
			return 1;

		default:
			if ((mark & MK_MSK) == APP_MK) {
				//mk_mon_debug_info(9);
				skip_segment();
				break;
			}
			if (RST_MK(mark)) {
				//mk_mon_debug_info(10);
				reset_prediction();
				break;
			}
			return 1;
			break;
		}		/* end switch */
	} while (1);

	return 0;
}
