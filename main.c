#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elevator.h"

#define ITER_PER_CYCLE 1000000000

bool terminate = false;
uint32_t cycle = 0;

int time_keeper(void);
uint8_t read_line(FILE *stream, char **out);
unsigned int *str_to_many_ui(char *str);

int main(void)
{
	// Initialise the timer
	pthread_t time_keeper_id;
	int retval = pthread_create(&time_keeper_id, NULL, &time_keeper, NULL);
	if (retval != 0)
	{
		printf("pthread_create returned %i\n", retval);
		return 1;
	}

	// Read scene data
	int **scene_data;
	FILE *scene_file;
	retval = fopen_s(&scene_file, "scene.txt", "r");
	if (retval != 0)
	{
		printf("fopen_s returned %i\n", retval);
		return 2;
	}
	
	char *raw_init_data;
	retval = read_line(scene_file, &raw_init_data);
	if (retval != 0)
	{
		printf("read_line returned %i\n", retval);
		return 3;
	}

	printf("%s\n", raw_init_data);

	unsigned int *init_data = str_to_many_ui(raw_init_data);
	if (init_data == NULL)
	{
		printf("str_to_many_ui returned NULL\n");
		return 4;
	}

	for (int i = 0; i < 2; i++)
	{
		printf("%u ", init_data[i]);
	}

	unsigned int levels = init_data[0];
	unsigned int people = init_data[1];
	person *queue = malloc(people * sizeof(person));

	for (int i = 0; i < people; i++)
	{
		char *raw_entity_data;
		retval = read_line(scene_file, &raw_entity_data);
		if (retval != 0)
		{
			printf("read_line returned %i\n", retval);
			return 5;
		}
		unsigned int *entity_data = str_to_many_ui(raw_entity_data);
		if (entity_data == NULL)
		{
			printf("str_to_many_ui returned NULL\n");
			return 6;
		}
		free(raw_entity_data);
	}
	

	while (!terminate) {};
	pthread_join(time_keeper_id, NULL);
	fclose(scene_file);
	free(raw_init_data); free(init_data); free(queue);

	return 0;
}

int time_keeper(void) 
{
	while (!terminate) 
	{
		for (int i = 0; i < ITER_PER_CYCLE; i++) {}; 
		cycle++;
		if (cycle == INT_MAX)
		{
			terminate = true;
		}
	}

	return 0;
}

uint8_t read_line(FILE *stream, char **out)
{
	long len = 2;
	*out = NULL;

	bool EOL = false;

	// Stores line without nul terminator
	while (!EOL)
	{
		int buffer = getc(stream);
		if (buffer == '\n' || buffer == EOF)
		{
			EOL = true;
			if (buffer == '\n')
			{
				fseek(stream, 1, SEEK_CUR);
			}

			break;
		}

		char *new_out = realloc(*out, (len)*sizeof(char));
		if (new_out == NULL)
		{
			return 1;
		}
		
		*out = new_out;
		(*out)[len-2] = (char) buffer;
		len++;
	}
	
	// Adds nul terminator 
	(*out)[len-2] = '\0';
	
	return 0;
}

unsigned int *str_to_many_ui(char *str)
{
	unsigned int *ints = NULL;
	int len = 0;
	char *buffer;
	char *saveptr;
	buffer = strtok_r(str, " ", &saveptr);

	while (buffer != NULL)
	{
		unsigned int *new_ints = realloc(ints, (len++)*sizeof(unsigned int));
		if (new_ints == NULL)
		{
			return NULL;
		}

		ints = new_ints;
		unsigned long ul = strtoul(buffer, NULL, 10);
		ints[len-1] = ul > INT_MAX ? INT_MAX : ul < 0 ? 0 : ul;
		buffer = strtok_r(NULL, " ", &saveptr);
	}

	return ints;
}