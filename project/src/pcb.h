#ifndef PCB_H
#define PCB_H

#include "shellmemory.h"
typedef struct pcb
{
    int pid; // how to make sure unique pid?
    program *p;
    int index;        // program counter
    struct pcb *next; // Use the named struct here
    int score;
} pcb;

#endif