#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "elevator.h"

bool terminate = false;
unsigned int cycle = 0;
unsigned int levels;

unsigned int num_requests = 0;
person **requests;

int time_keeper(void) 
{
	while (!terminate) 
	{
		sleep(1);
		cycle++;
		if (cycle == UINT_MAX)
		{
			terminate = true;
		}
	}

	return 0;
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