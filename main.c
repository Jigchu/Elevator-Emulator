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
unsigned long cycle = 0;

int time_keeper(void);
void read_line(FILE *stream, char **out);
unsigned int *split_ui(char *str);

int main(void)
{
	// Read scene data
	int **scene_data;
	FILE *scene_file;
	int retval = fopen_s(&scene_file, "scene.txt", "r");
	if (retval != 0)
	{
		printf("fopen_s returned %i\n", retval);
		exit(EXIT_FAILURE);
	}
	
	char *raw_init_data;
	read_line(scene_file, &raw_init_data);

	printf("%s\n", raw_init_data);

	unsigned int *init_data = split_ui(raw_init_data);
	for (int i = 0; i < 2; i++)
	{
		printf("%u ", init_data[i]);
	}

	printf("\n");

	unsigned int levels = init_data[0];
	unsigned int people = init_data[1];
	person *queue = malloc(people * sizeof(person));

	for (unsigned int i = 0; i < people; i++)
	{
		
	}

	// Initialise the timer
	pthread_t time_keeper_id;
	retval = pthread_create(&time_keeper_id, NULL, &time_keeper, NULL);
	if (retval != 0)
	{
		printf("pthread_create returned %i\n", retval);
		return 1;
	}

	while (!terminate) {};
	pthread_join(time_keeper_id, NULL);
	fclose(scene_file);
	free(raw_init_data); free(init_data); free(queue);

	exit(EXIT_SUCCESS);
}

int time_keeper(void) 
{
	while (!terminate) 
	{
		for (long i = 0; i < ITER_PER_CYCLE; i++) {}; 
		cycle++;
		if (cycle == ULONG_MAX)
		{
			terminate = true;
		}
	}

	return 0;
}

void read_line(FILE *stream, char **out)
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
			printf("Could not realloc (read_line)\n");
			exit(EXIT_FAILURE);
		}
		
		*out = new_out;
		(*out)[len-2] = (char) buffer;
		len++;
	}
	
	// Adds nul terminator 
	(*out)[len-2] = '\0';
	
	return;
}

unsigned int *split_ui(char *str)
{
	int len = 0;
	unsigned int *values = NULL;
	char *token, *last, *delim = " ";
	token = strtok_r(str, delim, &last);

	while (token != NULL)
	{
		unsigned int *temp = realloc(values, len+1 * sizeof(unsigned int));
		if (temp == NULL)
		{
			printf("Could not realloc (split_ui)\n");
			exit(EXIT_FAILURE);
		}
		
		values = temp;
		unsigned long value = strtoul(token, NULL, 10);
		values[len] = (unsigned int) (value > UINT_MAX ? UINT_MAX : value);
		token = strtok_r(NULL, delim, &last);
		len++;
	}

	return values;
}