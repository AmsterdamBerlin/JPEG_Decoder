#ifndef MEMMAP_H
#define MEMMAP_H

#define MEM_OFFSET (4*1024*1024)

//#define MEM_OFFSET 1207959552

volatile unsigned int *input_buffer = (unsigned int*) (shared_pt_REMOTEADDR + MEM_OFFSET);

#endif
