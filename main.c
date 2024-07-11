#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elevator.h"

#define ITER_PER_CYCLE 1000000000

bool terminate = false;
unsigned int cycle = 0;

int time_keeper(void);
void read_line(FILE *stream, char **out);
unsigned int *split_ui(char *str);
int av_cmp(void *context, const person *a, const person *b);

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

	unsigned int *init_data = split_ui(raw_init_data);

	char *raw_elevator_data;
	read_line(scene_file, &raw_elevator_data);
	unsigned int *elevator_data = split_ui(raw_elevator_data);
	elevator *lift = malloc(sizeof(elevator));
	if (lift == NULL)
	{
		printf("Could not allocate memory for lift\n");
		exit(EXIT_FAILURE);
	}
	
	lift->speed = elevator_data[0];
	lift->max_capacity = elevator_data[1];

	unsigned int levels = init_data[0];
	unsigned int num_people = init_data[1];
	person **queue = malloc(num_people * sizeof(person *));
	if (queue == NULL)
	{
		printf("Could not allocate memory for queue");
		exit(EXIT_FAILURE);
	}

	for (unsigned int i = 0; i < num_people; i++)
	{
		char *raw_person_data;
		read_line(scene_file, &raw_person_data);
		unsigned int *person_data = split_ui(raw_person_data);

		queue[i] = malloc(sizeof(person));
		if (queue[i] == NULL)
		{
			printf("Could not allocate memory for person at %i in queue\n", i);
			exit(EXIT_FAILURE);
		}

		(queue[i])->time = person_data[0];
		(queue[i])->direction = person_data[1] < 1 ? false : true;
		(queue[i])->start = person_data[2];
		(queue[i])->destination = person_data[3];
	}

	qsort_s(queue, num_people, sizeof(person *), &av_cmp, NULL);

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
		if (cycle == UINT_MAX)
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
		len++;
		unsigned int *temp = realloc(values, len * sizeof(int));
		if (temp == NULL)
		{
			printf("Could not realloc (split_ui)\n");
			exit(EXIT_FAILURE);
		}
		
		values = temp;
		unsigned long value = strtoul(token, NULL, 10);
		values[len-1] = (unsigned int) (value > UINT_MAX ? UINT_MAX : value);
		token = strtok_r(NULL, delim, &last);
	}

	return values;
}

// av stands for Action Value (Its just the time of occurrence)
int av_cmp(void *context, const person *a, const person *b)
{
	return (long long) a->time - (long long) b->time;
}