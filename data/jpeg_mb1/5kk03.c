//#include <stdio.h>

#include "5kk03.h"
#include "jpeg.h"
#include <global_memmap.h>
#include <5kk03-utils.h>
#include <hw_dma.h>
#include <comik.h>

extern int vld_count;
volatile unsigned int *input_buffer = (unsigned int*) (shared_pt_REMOTEADDR + 1024*1024*4);
volatile unsigned int *DMA_input1 = (unsigned int*)(mb1_cmemout0_BASEADDR);

DMA dma = {0, mb1_dma0_BASEADDR};
extern int vld_count;


/***************** paint ******************************/
void paint ()
{	
	
	volatile unsigned int *fb   = (unsigned int *)shared_pt_REMOTEADDR;
	volatile unsigned int *data = (unsigned int *)mb1_cmemout0_BASEADDR; 
	for ( int i = 0; i < 1024; i++) {
		data[i] = 0x55555555;
	}
	for ( int a  = 0; a  < 768; a++ ){
		hw_dma_send(&fb[a*1024], data, 1024, &dma, NON_BLOCKING);
		while(hw_dma_status(&dma));
	}

} 



/***************** initialize the input buffer by DMA *************/
void DMA_initial()
{	

	hw_dma_receive(DMA_input1,input_buffer, DMA_size, &dma, BLOCKING);
	while(hw_dma_status(&dma));

	input_buffer += DMA_size;

	vld_count = 0;
}


unsigned int FGETC()
{		
	//mk_mon_debug_info(529367213);
	// convert from little to BIG endian
	unsigned int c = ((SwapEndian(DMA_input1[vld_count / 4]) << (8 * (vld_count % 4))) >> 24) & 0x00ff;
	
	vld_count++;
	// input the data from remote memory when buffer is completely consumed. 
	if(vld_count == 4*DMA_size)
	{	
		//while(hw_dma_status(&dma));
		hw_dma_receive(DMA_input1,input_buffer, DMA_size, &dma, NON_BLOCKING);
		while(hw_dma_status(&dma));
		input_buffer += DMA_size;
		//mk_mon_debug_info(0xbebecafe);
		vld_count = 0;
	}
	
	return c;
}

int FSEEK(int offset, int start)
{	
	

	vld_count += offset + (start - start);	/* Just to use start... */

	while(vld_count >= 4*DMA_size)
	{	
		// Wait for DMA to be ready 
		hw_dma_receive(DMA_input1,input_buffer, DMA_size, &dma, NON_BLOCKING);
		while(hw_dma_status(&dma));
		input_buffer += DMA_size;
		vld_count -= 4*DMA_size;
	}
		
	
	return 0;
}

int FTELL()
{
	return vld_count;
}
