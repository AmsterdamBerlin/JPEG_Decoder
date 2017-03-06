/*-----------------------------------------*/
/* File : table_vld.c, utilities for jfif view */
/* Author : Pierre Guerrier, march 1998	   */
/*-----------------------------------------*/

//#include <stdlib.h>
//#include <stdio.h>

#include "jpeg.h"

/*--------------------------------------*/
/* private huffman.c defines and macros */
/*--------------------------------------*/

/* Memory size of HTables; */
#define MAX_SIZE(hclass)		((hclass)?162:14)

/*--------------------------------------*/
/* some static structures for storage */
/*--------------------------------------*/

static unsigned char DC_Table0[MAX_SIZE(DC_CLASS)], DC_Table1[MAX_SIZE(DC_CLASS)];

static unsigned char AC_Table0[MAX_SIZE(AC_CLASS)], AC_Table1[MAX_SIZE(AC_CLASS)];

static unsigned char *HTable[4] = {
	&DC_Table0[0], &DC_Table1[0],
	&AC_Table0[0], &AC_Table1[0]
};

static int MinCode[4][16];
static int MaxCode[4][16];
static int ValPtr[4][16];

/*----------------------------------------------------------*/
/* Loading of Huffman table, with leaves drop ability	    */
/*----------------------------------------------------------*/

//int load_huff_tables(FILE * fi)
int load_huff_tables()
{
	char aux;
	int size, hclass, id, max;
	int LeavesN, LeavesT, i;
	int AuxCode;

	size = get_size();	/* this is the tables' size */
	size -= 2;
	
	while (size > 0) {

		aux = FGETC();
		hclass = first_quad(aux);	/* AC or DC */
		id = second_quad(aux);	/* table no */
		if (id > 1) {
			return -1;
		}
		id = HUFF_ID(hclass, id);
		size--;

		LeavesT = 0;
		AuxCode = 0;

		for (i = 0; i < 16; i++) {
			LeavesN = FGETC();

			ValPtr[id][i] = LeavesT;
			MinCode[id][i] = AuxCode * 2;
			AuxCode = MinCode[id][i] + LeavesN;

			MaxCode[id][i] = (LeavesN) ? (AuxCode - 1) : (-1);
			LeavesT += LeavesN;
		}
		size -= 16;

		if (LeavesT > MAX_SIZE(hclass)) {
			max = MAX_SIZE(hclass);
		} else
			max = LeavesT;

		for (i = 0; i < max; i++)
			HTable[id][i] = FGETC();	/* load in raw order */

		for (i = max; i < LeavesT; i++)
			FGETC();	/* skip if we don't load */
		size -= LeavesT;

	}			/* loop on tables */
	return 0;
}

/*-----------------------------------*/
/* extract a single symbol from file */
/* using specified huffman table ... */
/*-----------------------------------*/

//unsigned char get_symbol(FILE * fi, int select)
unsigned char get_symbol( int select)
{
	long code = 0;
	int length;
	int index;

	for (length = 0; length < 16; length++) {
		code = (2 * code) | get_one_bit();
		if (code <= MaxCode[select][length])
			break;
	}

	index = ValPtr[select][length] + code - MinCode[select][length];

	if (index < MAX_SIZE(select / 2))
		return HTable[select][index];

	return 0;
}
