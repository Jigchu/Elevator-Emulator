#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "elevator.h"

#define ITER_PER_CYCLE 1000000000

bool terminate = false;
uint64_t cycle = 0;

int time_keeper(void);
uint8_t read_line(FILE *stream, char **out);

int main(void)
{
	// Initialise the timer
	pthread_t time_keeper_id;
	int retval = pthread_create(&time_keeper_id, NULL, &time_keeper, NULL);
	if (retval != 0)
	{
		printf("pthread_create returned %i", retval);
		return 1;
	}

	// Read scene data
	int **scene_data;
	FILE *scene_file;
	retval = fopen_s(&scene_file, "scene.txt", "r");
	if (retval != 0)
	{
		printf("fopen_s returned %i", retval);
		return 2;
	}
	
	char *init_data;
	retval = read_line(scene_file, &init_data);
	if (retval != 0)
	{
		printf("read_line returned %i", retval);
		return 3;
	}
	printf(init_data);

	while (!terminate) {};
	pthread_join(time_keeper_id, NULL);

	return 0;
}

int time_keeper(void) 
{
	while (!terminate) 
	{
		for (int i = 0; i < ITER_PER_CYCLE; i++) {}; 
		cycle++;
	}

	return 0;
}

uint8_t read_line(FILE *stream, char **out)
{
	long len = 1;
	*out = NULL;

	bool EOL = false;

	// Stores line without nul terminator
	while (!EOL)
	{
		int buffer = getc(stream);
		if (buffer == '\n' || buffer == EOF)
		{
			EOL = true;
			fseek(stream, 1, SEEK_CUR);
			break;
		}

		char *new_out = realloc(*out, (len)*sizeof(char));
		if (new_out == NULL)
		{
			return 1;
		}
		
		*out = new_out;
		(*out)[len-1] = (char) buffer;
		len++;
	}

	// Adds nul terminator 
	(*out)[len-1] = '\0';
	
	return 0;
}