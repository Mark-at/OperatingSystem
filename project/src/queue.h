#ifndef QUEUE_H
#define QUEUE_H

#include "pcb.h"
typedef struct
{
    pcb *head; // that is all
} ready;

void enqueueAging(ready *rq, pcb *pcb);
void enqueue(ready *rq, pcb *pcb);

#endif