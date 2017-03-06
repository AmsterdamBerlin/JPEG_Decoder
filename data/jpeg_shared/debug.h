#ifndef DEBUG_H
#define DEBUG_H

// Contains a bunch of functions and macros for use in debugging and benchmarking.
// These functions and macros output data in the form of error messages with error
// codes 0x0D**. These error messages can be parsed to make meaningful data using 
// functions in dbgParser.py.

// DBG levels:
// 0: Nothing. All these functions and macros do nothing.
// 1: Simple inline time and data reporting functions only, no string printing.
// 2: All from DBG level 1 + Basic string printing (mk_puts(), etc), no formatted strings.
// 3: All from DBG level 2 + formated string printing (using mk_printf())
// Default level (if DBG is undefined) is 0

#ifndef DBG
#define DBG 1
#endif

#if (DBG >= 1)

	#include <comik.h>

	#define DBG_TIME_BLOCK_START_ID 0xD20
	#define DBG_TIME_BLOCK_END_ID 0xD21
	#define DBG_TIME_EVENT_ID 0xD23
	#define DBG_TIME_CORE_START_ID 0xD30
	#define DBG_TIME_CORE_END_ID 0xD31
	#define DBG_TIME_CORE_PAUSE_ID 0xD32
	#define DBG_TIME_CORE_UNPAUSE_ID 0xD33

	#define DBG_DATA_POINT 0xD40 // NOTE: Data point uses the entire 0xD40-0xD50 range

	// Time event reporting function, simply reports a time with the given id
	inline void dbg_time_event(const int id){
		mk_mon_error(DBG_TIME_EVENT_ID, id);
	}

	// Call this at the start of a block of code that you want to time 
	// NOTE: Each timed block must have a unique id!
	inline void dbg_time_block_start(const int id){
		mk_mon_error(DBG_TIME_BLOCK_START_ID, id);
	}

	// Call this at the end of a block of code that you want to time
	// NOTE: Each timed block must have a unique id!
	inline void dbg_time_block_end(const int id){
		mk_mon_error(DBG_TIME_BLOCK_END_ID, id);
	}

	// call this from the beginning of your core main to measure the core run time
	inline void dbg_time_core_start(){
		mk_mon_error(DBG_TIME_CORE_START_ID,0);
	}

	// call this from the end of your core main to measure the core run time 
	inline void dbg_time_core_end(){
		mk_mon_error(DBG_TIME_CORE_END_ID,0);
	}

	// Call this in your core main if/where you want to pause the core run timing
	inline void dbg_time_core_pause(){
		mk_mon_error(DBG_TIME_CORE_PAUSE_ID,0);
	}

	// Call this in your core main if/where you want to unpause the core run timing
	inline void dbg_time_core_unpause(){
		mk_mon_error(DBG_TIME_CORE_UNPAUSE_ID,0);	
	}

	// Data point reporting function
	// The dataID must be a value from 0-15 (one nibble), multiple data points
	// can be reported under the same id to create a single data set
	inline void dbg_report_data_point(unsigned char dataID, int data){
		mk_mon_error(DBG_DATA_POINT | (dataID & 0xf), data);
	}
#else
	inline void dbg_time_event(const int id){}
	inline void dbg_time_block_start(const int id){}
	inline void dbg_time_block_end(const int id){}
	inline void dbg_time_core_start(){}
	inline void dbg_time_core_end(){}
	inline void dbg_time_core_pause(){}
	inline void dbg_time_core_unpause(){}
	inline void dbg_report_data_point(unsigned char dataID, int data){}
#endif

#if (DBG >= 2)
	#define DBG_STRING_START 0xD10
	#define DBG_STRING_MID 0xD11
	#define DBG_STRING_END 0xD12
	#define DBG_STRING_NUMERIC 0xD13

	// Log a string and include file, line and function where the log was invoked.
	#define MK_LOG(s) { 		\
		mk_puts(__FILE__ ":");	\
		mk_putn(__LINE__);		\
		mk_puts(":");			\
		mk_puts(__func__);		\
		mk_puts(": " s "\n"); 	\
	}

	// Benchmark the code provided as an argument
	#define BENCHMARK(x) { \
		TIME t1, t2; \
		t1 = hw_tifu_systimer_get(); \
		x; \
		t2 = hw_tifu_systimer_get(); \
		TIME diff = t2 - t1; \
		MK_PUTT("Anonymous", diff, 1) \
	}

	// Print a timing info.
	#define MK_PUTT(id, total, count) { 						\
			MK_LOG("Results of benchmark with id '" id "'");	\
			mk_puts("\ttotal: ");								\
			mk_putnln(total);									\
			if (count != 0) {									\
				mk_puts("\tavg: ");								\
				mk_putnln(total/count);							\
				mk_puts("\tcount: ");							\
				mk_putnln(count);								\
			}													\
			else {												\
				mk_puts("count: 0");							\
			}													\
			mk_puts("\n");										\
		}

	#define DECLARE_TIMING(s)  		TIME timeStart_##s; TIME timeDiff_##s; int timeTally_##s = 0; int countTally_##s = 0
	#define START_TIMING(s)    		timeStart_##s = hw_tifu_systimer_get()
	#define STOP_TIMING(s)     		timeDiff_##s = (hw_tifu_systimer_get() - timeStart_##s); timeTally_##s += timeDiff_##s; countTally_##s++
	#define GET_TIMING(s)      		LO_64(timeDiff_##s)
	#define GET_AVERAGE_TIMING(s)   (countTally_##s ? LO_64(timeTally_##s) / countTally_##s : 0)
	#define CLEAR_AVERAGE_TIMING(s) timeTally_##s = 0; countTally_##s = 0
	#define REPORT_TIMING(s) 		MK_PUTT(#s, LO_64(timeTally_##s), LO_64(countTally_##s))

	// print a string
	void mk_puts(const char *string){
		int size = 0;
		for(;string[size] != '\0'; size++);
		int i;
		
		// Convert characters to integers in groups of 4 (1 int = 4 bytes = 4 chars). Reading the char* as an int*.
		mk_mon_error(DBG_STRING_START, *((int *)(string)));
		for(i = 4; i < size - 4; i += 4) {
			mk_mon_error(DBG_STRING_MID, *((int *)(string + i)));
		}

		// Convert the remaining characters and pad them with null characters to form a group of 4.
		int remaining = 0;
		for(int j = i; j < i + 4; ++j) {
			remaining |=  (j < size ? string[j] : '\0')  << (8 * (j - i));
		}
		mk_mon_error(DBG_STRING_END, remaining);
	}

	// print an int
	inline void mk_putn(const int num) {
		mk_mon_error(DBG_STRING_NUMERIC, num);
	}

	// print an int and put a new line after.
	inline void mk_putnln(const int num) {
		mk_putn(num);
		mk_puts("\n");
	}

#else 
	inline void mk_puts(const char *string){}
	inline void mk_putn(const int num) {}
	inline void mk_putnln(const int num) {}

	#define MK_LOG(s)  					{}
	#define BENCHMARK(x) 				{}
	#define MK_PUTT(id, total, count)	{}
	#define DECLARE_TIMING(s)  			{}
	#define START_TIMING(s)    			{}
	#define STOP_TIMING(s)     			{}
	#define GET_TIMING(s)     			(0)
	#define GET_AVERAGE_TIMING(s)   	(0)
	#define CLEAR_AVERAGE_TIMING(s) 	{}
	#define REPORT_TIMING(s) 			{}

#endif

#if (DBG >= 3)
	#include <string.h>
	#include <stdarg.h>
	#include <stdio.h>

	// Currently works only on cores 1 and 2. 
	// Can print strings up to 512 chars.
	// Print formatted text.
	int mk_printf(const char *format, ...) {
		va_list arg;
		int done;

		char out[512];

		va_start(arg, format);

		done = vsprintf(out, format, arg);

		va_end(arg);

		mk_puts(out);

		return done;
	}

	// formatted log
	#define MK_FLOG(fmt, ...) { \
		mk_printf("%s:%d:%s: " fmt "\n", __FILE__ , __LINE__, __func__, __VA_ARGS__); \
	}
#else
	inline int mk_printf(const char *format, ...){
		return 0;
	}

	#define MK_FLOG(fmt, ...) {}
#endif

#endif
