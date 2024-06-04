#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct elevator
{
	uint8_t max_capacity;
	uint8_t curr_capacity;
	double level;
	uint8_t speed;
	bool direction;
} elevator_t;

typedef struct person {
	unsigned long time;
	bool direction;
	uint8_t start;
	uint8_t destination;
} person;