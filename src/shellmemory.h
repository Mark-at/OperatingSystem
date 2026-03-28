#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

// #define MEM_SIZE 1000
// #define fS_lines 120

#ifndef fS_lines
#define fS_lines 18
#endif

#ifndef MEM_SIZE
#define MEM_SIZE 10
#endif

void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
char *fS_getter(int phyIndex, int time);
int fS_set_value(char *tripletVal[]);
int fS_evict();
int hasNoSpace();

typedef struct
{
    int numOfLines;
    char *lines[];
} program; // deprecated now, we store programlines in frameStore

#endif