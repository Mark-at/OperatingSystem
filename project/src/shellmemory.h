#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

#define MEM_SIZE 1000
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
typedef struct
{
    int numOfLines;
    char *lines[];
} program;

#endif