# Elevator-Emulator
A program to emulate elevators

## Input

```
[Number of levels] [Number of people] // Initial Variables
[Speed] [Max Capacity] // Elevator Data
[Cycle of appearance] [Direction] [Start Level] [Destination Level] // Entity Data
...
[Cycle of appearance] [Direction] [Start Level] [Destination Level] // Last person
```
*One Cycle is the time taken for a 1,000,000,000 iterations defined using `#define` under the name `ITER_PER_CYCLE`