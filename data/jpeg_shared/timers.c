#include <comik.h>
#include <hw_tifu.h>
#include "timers.h"
#include "jpeg.h"

Timer_t timer[MAX_TIMERS];


void timer_start(int timer_id)
{
	for (int i=0;i<MAX_TIMERS;i++)
	{
		if(i == timer_id)
		{
			timer[i].start = hw_tifu_systimer_get();
		}
	}
}

void timer_stop(int timer_id)
{
	for (int i=0; i<MAX_TIMERS; i++)
	{
		if(i == timer_id)
		{
			timer[i].stop = hw_tifu_systimer_get();
			TIME diff = timer[i].stop - timer[i].start - TIMER_ACCESS_TIME;
			int tdiff = LO_64(diff);
			timer[i].tot_time += tdiff;
			
		}
	}
}

void timer_print()
{
	for (int i=0; i<MAX_TIMERS; i++)
	{
		mk_mon_debug_info(i);
		mk_mon_debug_info(timer[i].tot_time);
	}
}