#include "queue.h"
#include <stdio.h>

void enqueueAging(ready *rq, pcb *p) // for SJF and Aging (in SJF, by default the score is length)
{
    if (rq->head == NULL || p->score <= rq->head->score) // 1)for queue w/ 1 pcb, 2) for tie
    {
        p->next = rq->head;
        rq->head = p;
        return;
    }

    pcb *temp = rq->head;
    while (temp->next != NULL && temp->next->score <= p->score)
    {
        temp = temp->next;
    }
    p->next = temp->next;
    temp->next = p;
}

void enqueue(ready *rq, pcb *p) // for FCFS, RR and RR30
{
    p->next = NULL;

    if (rq->head == NULL)
    {
        rq->head = p;
        return;
    }

    pcb *temp = rq->head;
    while (temp->next != NULL)
    {
        temp = temp->next;
    }
    temp->next = (struct pcb *)p; // san chu casts
}