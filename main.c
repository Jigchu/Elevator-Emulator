#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elevator.h"

#define ITER_PER_CYCLE 1000000000

bool terminate = false;
unsigned int cycle = 0;
unsigned int num_requests = 0;

unsigned int levels;
person **requests;

pthread_mutex_t request_mutex;
pthread_mutex_t serving_mutex;

int time_keeper(void);
void read_line(FILE *stream, char **out);
unsigned int *split_ui(char *str);
int st_cmp(void *context, const void *a, const void *b);
void person_scheduler(long long *args);
void elevator_manager(elevator *lift);
void elevator_movement(elevator *lift);

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
	levels = init_data[0];
	unsigned int people_num = init_data[1];
	free(raw_init_data); free(init_data);

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
	free(raw_elevator_data); free(elevator_data);

	person **queue = malloc(people_num * sizeof(person *));
	if (queue == NULL)
	{
		printf("Could not allocate memory for queue");
		exit(EXIT_FAILURE);
	}

	for (unsigned int i = 0; i < people_num; i++)
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

		(queue[i])->start_time = person_data[0];
		(queue[i])->direction = person_data[1] > 0;
		(queue[i])->start = person_data[2];
		(queue[i])->destination = person_data[3];
		(queue[i])->retrieved = false;
		free(raw_person_data); free(person_data);
	}

	fclose(scene_file);

	qsort_s(queue, people_num, sizeof(person *), &st_cmp, NULL);

	// Initialise the timer
	pthread_t time_keeper_id;
	retval = pthread_create(&time_keeper_id, NULL, &time_keeper, NULL);
	if (retval != 0)
	{
		printf("pthread_create returned %i (time_keeper)\n", retval);
		exit(EXIT_FAILURE);
	}

	// Initialise the request schduler
	pthread_mutex_init(&request_mutex, NULL);
	pthread_t person_scheduler_id;
	long long person_scheduler_args[2] = {(long long) people_num, (long long) queue};
	retval = pthread_create(&person_scheduler_id, NULL, &person_scheduler, person_scheduler_args);
	if (retval != 0)
	{
		printf("pthread_create returned %i (person_scheduler)\n", retval);
		exit(EXIT_FAILURE);
	}

	// Initialise the elevator manager
	pthread_t elevator_manager_id;
	pthread_mutex_init(&serving_mutex, NULL);
	retval = pthread_create(&elevator_manager_id, NULL, &elevator_manager, lift);
	if (retval != 0)
	{
		printf("pthread_create returned %i (elevator_manager)\n", retval);
		exit(EXIT_FAILURE);
	}

	while (!terminate)
	{
		printf("Cycle: %u\n", cycle);
		for (int i = 0; i < num_requests; i++)
		{
			printf("%u ", requests[i]->start_time);
		}
		printf("\n");
		printf("%u, %u, %s\n", lift->curr_level, lift->next_level, (lift->direction ? "up" : "down"));
		for (int i = 0; i < lift->curr_capacity; i++)
		{
			printf("%u: %u %u %u\n", i, lift->serving[i]->start, lift->serving[i]->destination, lift->serving[i]->retrieved);
		}

		sleep(1);
	}

	pthread_join(time_keeper_id, NULL);
	free(lift); free(queue);

	exit(EXIT_SUCCESS);
}

int time_keeper(void) 
{
	while (!terminate) 
	{
		for (long i = 0; i < ITER_PER_CYCLE; i++); 
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
	unsigned int len = 0;
	*out = NULL;
	int buffer = fgetc(stream);
	
	while (buffer != '\n' && buffer != EOF)
	{
		char *tmp = realloc(*out, (len + 2) * sizeof(char));
		if (tmp == NULL)
		{
			printf("Could not realloc (read_line)\n");
			exit(EXIT_FAILURE);
		}

		*out = tmp;
		(*out)[len] = buffer;
		len++;

		buffer = fgetc(stream);
	}

	if (len != 0)
	{
		(*out)[len] = '\0';
	}

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

// Compares the start times of the people
int st_cmp(void *context, const void *a, const void *b)
{
	const person *p1 = *(person **) a;
	const person *p2 = *(person **) b;
	long long result = ((long long) p1->start_time - (long long) p2->start_time) * -1;

	return result > INT_MAX ? INT_MAX : result < INT_MIN ? INT_MIN : (int) result;
}

// Keeps track of requests placed
void person_scheduler(long long *args)
{
	unsigned int num_people = (unsigned int) args[0];
	person **queue = (person **) args[1];
	requests = malloc(1 * sizeof(person *));
	if (requests == NULL)
	{
		printf("Could not allocate space for requests (person_scheduler)\n");
		exit(EXIT_FAILURE);
	}

	while (num_people)
	{
		if (cycle >= (queue[num_people-1])->start_time)
		{
			pthread_mutex_lock(&request_mutex);
			person **tmp = realloc(requests, (num_requests+1) * sizeof(person *));
			if (tmp == NULL)
			{
				printf("Could not reallocate memory for requests (person_scheduler)\n");
				exit(EXIT_FAILURE);
			}

			requests = tmp;
			requests[num_requests] = queue[num_people-1];
			num_requests++; num_people--;
			pthread_mutex_unlock(&request_mutex);
		}
	}

	return;
}

void elevator_manager(elevator *lift)
{
	// Initialising lift variables
	lift->curr_capacity = 0;
	lift->curr_level = 1;
	lift->next_level = 1;
	lift->direction = true;
	lift->serving = malloc(sizeof(person *));
	if (lift->serving == NULL)
	{
		printf("Could not allocate memory for lift->serving (elevator_manager)\n");
		exit(EXIT_FAILURE);
	}
	

	// Initialising elevator movement
	pthread_t elevator_movement_id;
	int retval = pthread_create(&elevator_movement_id, NULL, &elevator_movement, lift);
	if (retval != 0)
	{
		printf("pthread_create returned %i (elevator_movement)\n", retval);
		exit(EXIT_FAILURE);
	}

	// Algorithm that manages lift's movement
	while (!terminate)
	{
		// Prevents elevator from taking new requests if the elevator is full
		if (lift->curr_capacity == lift->max_capacity)
		{
			continue;
		}

		for (int i = 0; i < num_requests; i++)
		{
			person *request = requests[i];
			if (request->start == lift->curr_level)
			{
				continue;
			}
			
			bool direction_to_request = request->start > lift->curr_level;
			
			// Takes on the rrequest
			if (direction_to_request == lift->direction)
			{
				pthread_mutex_lock(&serving_mutex);

				person **tmp = realloc(lift->serving, (lift->curr_capacity+1) * sizeof(person *));
				if (tmp == NULL)
				{
					printf("Could not reallocate memory for lift-serving (elevator_manager)\n");
					exit(EXIT_FAILURE);
				}

				lift->serving = tmp;
				(lift->serving)[lift->curr_capacity] = request;
				lift->curr_capacity++;

				pthread_mutex_unlock(&serving_mutex);

				pthread_mutex_lock(&request_mutex);
				
				for (int j = i; j < num_requests; j++)
				{
					requests[j] = requests[j+1];
				}

				num_requests--;

				tmp = realloc(requests, (num_requests == 0 ? 1 : num_requests) * sizeof(person *));
				if (tmp == NULL)
				{
					printf("Could not reallocate memory for requests (elevator_manager)\n");
					exit(EXIT_FAILURE);					
				}
				requests = tmp;

				pthread_mutex_unlock(&request_mutex);
			}
		}
	}

	return;
}

// In charge of the elevator's movement and destination
void elevator_movement(elevator *lift)
{
	unsigned int prev_cycle = cycle;

	while (!terminate)
	{
		if (lift->next_level != lift->curr_level)
		{
			lift->direction = lift->next_level > lift->curr_level;
			if (cycle != prev_cycle)
			{
				unsigned int levels_traversed = (cycle-prev_cycle) * lift->speed;
				lift->curr_level += lift->direction ? levels_traversed : (int) levels_traversed * -1;
				prev_cycle = cycle;
			}
		}

		// Drops off and picks up requests
		for (int i = 0; i < lift->curr_capacity; i++)
		{
			person *request = lift->serving[i];
			int destination = !request->retrieved ? request->start : request->destination; /* Where the elvator has to go to retrieve or complete the request */
			if (destination == lift->curr_level)
			{
				if (!request->retrieved)
				{
					request->retrieved = true;
					continue;
				}
				
				pthread_mutex_lock(&serving_mutex);

				for (int j = i; j < lift->curr_capacity; j++)
				{
					lift->serving[j] = lift->serving[j+1];
				}

				lift->curr_capacity--;

				person **tmp = realloc(lift->serving, (lift->curr_capacity == 0 ? 1 : lift->curr_capacity) * sizeof(person *));
				if (tmp == NULL)
				{
					printf("Could not reallocate memory for lift->serving (elevator_movement)\n");
					exit(EXIT_FAILURE);					
				}
				lift->serving = tmp;

				pthread_mutex_unlock(&serving_mutex);
			}
			
		}

		// Decides the next level the elevator should travel to
		unsigned int closest_above = levels; /*Closest level above lift with a request*/
		unsigned int closest_below = 1; /*Closest level below lift with a request*/

		for (int i = 0; i < lift->curr_capacity; i++)
		{
			person *request = lift->serving[i];
			int destination = !request->retrieved ? request->start : request->destination; /* Where the elvator has to go to retrieve or complete the request */
			
			if (destination == lift->curr_level)
			{
				continue;
			}
			
			bool direction_to_request = destination > lift->curr_level;
			closest_above = direction_to_request && destination < closest_above ? destination : closest_above;
			closest_below = !direction_to_request && destination > closest_below ? destination : closest_below;		
		}
		
		if (lift->direction)
		{
			lift->next_level = closest_above == lift->curr_level ? closest_below : closest_above;
		}
		else
		{
			lift->next_level = closest_below == lift->curr_level ? closest_above : closest_below;
		}
	}
	
	return;
}