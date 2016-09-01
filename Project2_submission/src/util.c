/*
 * util.c
 *
 *  Created on: Nov 16, 2015
 *      Author: nikita
 */
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include<string.h>    //strlen

int split(char * str, char delim, char ***array, int *length)
{
	char *p;
	char **res;
	int count = 0;
	int k = 0;

	p = str;
	// Count occurance of delim in string
	while ((p = strchr(p, delim)) != NULL) {
		*p = 0; // Null terminate the deliminator.
		p++; // Skip past our new null
		count++;
	}

	// allocate dynamic array
	res = calloc(1, (count + 1) * sizeof(char *));
	if (!res)
		return -1;

	p = str;
	for (k = 0; k <= count; k++) {
		if (*p)
			res[k] = p;  // Copy start of string
		p = strchr(p, 0);    // Look for next null
		p++; // Start of next string
	}

	*array = res;
	*length = count;

	return 0;
}


void uppercase( char *sPtr )
{
	char upper[20];
	int i;
	strcpy(upper,sPtr);

    for(i=0;i< (int)(strlen(sPtr));i++)
    	upper[i]=toupper(upper[i]);
    //printf("%zu\n",strlen(sPtr));
    strcpy(sPtr,upper);
}
