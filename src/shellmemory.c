#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

struct memory_struct
{
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];
int lruCounter[fS_lines / 3];
const char *fS[fS_lines]; // page store, must be in gr of 3 ele

// Helper functions
int match(char *model, char *var)
{
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++)
    {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len)
    {
        return 1;
    }
    else
        return 0;
}

// Shell memory functions

void mem_init()
{
    int i;
    for (i = 0; i < MEM_SIZE; i++)
    {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
    for (i = 0; i < fS_lines; i++) // initialize frame store
    {
        fS[i] = NULL;
    }
    for (i = 0; i < fS_lines / 3; i++) // initialize the table storing the last time a page is accessed
    {
        lruCounter[i] = 0;
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in)
{
    int i;

    for (i = 0; i < MEM_SIZE; i++)
    {
        if (strcmp(shellmemory[i].var, var_in) == 0)
        {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    // Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++)
    {
        if (strcmp(shellmemory[i].var, "none") == 0)
        {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    return;
}
int fS_set_value(char *tripletVal[]) // takes 3 lines of program code as triplet and store them to frame store
// returns the index (which is a multiple of 3) of the first line of that page in frame store
{
    int ptEntry = -2;
    for (int i = 0; i < fS_lines; i += 3)
    {
        if (fS[i] == NULL)
        {
            ptEntry = i;
            for (int j = 0; j < 3; j++)
            {
                fS[i + j] = tripletVal[j];
            }
            break;
        }
    }
    return ptEntry;
}

int hasNoSpace()
{
    for (int i = 0; i < fS_lines; i += 3)
    {
        if (fS[i] == NULL)
        {
            return 0;
        }
    }
    return 1;
}
char *fS_getter(int phyIndex, int time)
{
    lruCounter[phyIndex / 3] = time;
    return strdup(fS[phyIndex]);
}

int fS_evict()
{
    int min = 10000;
    int frame = 0;
    for (int i = 0; i < fS_lines / 3; i++)
    {
        if (lruCounter[i] < min)
        {
            min = lruCounter[i];
            frame = i; // tells you which frame is the least recently used
        }
    }
    int line = 3 * frame; // index of the first line of that frame in framestore

    printf("%s", fS[line]);
    fS[line] = NULL;

    printf("%s", fS[line + 1]);
    fS[line + 1] = NULL;

    printf("%s", fS[line + 2]);
    fS[line + 2] = NULL;
    return line;
}

// get value based on input key
char *mem_get_value(char *var_in)
{
    int i;

    for (i = 0; i < MEM_SIZE; i++)
    {
        if (strcmp(shellmemory[i].var, var_in) == 0)
        {
            return strdup(shellmemory[i].value);
        }
    }
    return "Variable does not exist";
}