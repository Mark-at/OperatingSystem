#ifndef PCB_H
#define PCB_H

#include "shellmemory.h"
typedef struct pcb
{
    int pid;
    program *p; // now deprecated, but kept for convenience
    int numOfCmd;
    int *pageTable;
    int index;        // program counter
    struct pcb *next; // Use the named struct here
    int score;
    char *fileName; // a string
} pcb;

#endif