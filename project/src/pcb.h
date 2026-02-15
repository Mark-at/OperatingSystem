typedef struct
{
    int pid; // how to make sure unique pid?
    char **p;
    int index;
    pcb *next;
    // delta
    int numOfLines;
} pcb;