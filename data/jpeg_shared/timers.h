#ifndef TIMERS_H
#define TIMERS_H

/* File containing definition for all timers */

#define TIMERS 				1
#define MAX_TIMERS			4
#define TIMER_ACCESS_TIME		55

/*Timer definitions*/
#define TIMER_1	0
#define TIMER_2	1



typedef struct 
{
	/* data */
	TIME start, stop;
	unsigned int tot_time;
}Timer_t;


void timer_start(int timer_id);
void timer_stop(int timer_id);
void timer_print();


#endif
