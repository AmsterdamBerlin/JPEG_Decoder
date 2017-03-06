#include <comik.h>
#include <global_memmap.h>
#include <5kk03-utils.h>
//#include "memmap.h"

#include "timers.h"
#include "jpeg.h"
#include "debug.h"
//#include "surfer.jpg.h"

int main (void)
{		
	// Sync with the monitor.
	mk_mon_sync();
	// start core timer
	dbg_time_core_start();
	// Enable stack checking.
	start_stack_check();
	//paint();
	mk_mon_debug_info(1000);
	
	// Start decoding the JPEG.
    
	//mk_mon_debug_info(LO_64(*input_buffer));
	if( JpegToBmp()) {
        mk_mon_debug_info(2000);
	}
    else 
    mk_mon_debug_info(3000);

	//end core timer
	dbg_time_core_end();
	timer_print();
	// Signal the monitor we are done.
	mk_mon_debug_tile_finished();
	return 0;
}
