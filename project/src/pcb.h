#ifndef PCB_H
#define PCB_H

#include "shellmemory.h"
typedef struct
{
    int pid; // how to make sure unique pid?
    // char **p;
    program *p;
    int index; // program counter
    struct pcb *next;
    // delta for later tasks
    // int numOfLines;
} pcb;

#endif