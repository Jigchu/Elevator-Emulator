#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct person {
	unsigned int start_time;
	bool direction;
	uint8_t start;
	uint8_t destination;
	bool retrieved;
} person;

typedef struct elevator
{
	uint8_t max_capacity;
	uint8_t curr_capacity;
	unsigned int curr_level;
	unsigned int next_level;
	uint8_t speed; /* Number of levels per cycle */
	bool direction;
	person **serving;
} elevator;
