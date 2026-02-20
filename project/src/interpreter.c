#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "shellmemory.h"
#include "shell.h"
#include "pcb.h"
#include "queue.h"

int MAX_ARGS_SIZE = 5;

int badcommand()
{
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist()
{
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int source(char *script);
int badcommandFileDoesNotExist();
int echo(char *toBeEchoed);
int my_ls();
int my_touch(char *fName);
int my_mkdir(char *dirname);
int my_cd(char *dirname);
int run(char *input[]);
int scheduler(ready rQ);
int exec(char *arg[], int argS);

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size)
{
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE)
    {
        return badcommand();
    }

    for (i = 0; i < args_size; i++)
    { // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0)
    {
        // help
        if (args_size != 1)
            return badcommand();
        return help();
    }
    else if (strcmp(command_args[0], "exec") == 0)
    {
        if (args_size < 3 || args_size > 5)
        {
            return badcommand();
        }
        if (strcmp(command_args[args_size - 1], "FCFS") != 0 && strcmp(command_args[args_size - 1], "SJF") != 0 && strcmp(command_args[args_size - 1], "RR") != 0) // for 1.2.2 we just check FCFS
        {
            return badcommand();
        }
        for (int i = 1; i < args_size - 2; i++)
        {
            for (int j = i + 1; j < args_size; j++) // per instruction, if program1 is same as p2, error
            {
                if (!strcmp(command_args[i], command_args[j]))
                {
                    return badcommand();
                }
            }
        }
        return exec(command_args, args_size);
    }
    else if (strcmp(command_args[0], "quit") == 0)
    {
        // quit
        if (args_size != 1)
            return badcommand();
        return quit();
    }
    else if (strcmp(command_args[0], "set") == 0)
    {
        // set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);
    }
    else if (strcmp(command_args[0], "print") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);
    }
    else if (strcmp(command_args[0], "source") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);
    }
    else if (strcmp(command_args[0], "echo") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_ls") == 0)
    {
        if (args_size != 1)
            return badcommand();
        return my_ls();
    }
    else if (strcmp(command_args[0], "my_touch") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return my_touch(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_cd") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return my_cd(command_args[1]);
    }
    else if (strcmp(command_args[0], "my_mkdir") == 0)
    {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);
    }
    else if (strcmp(command_args[0], "run") == 0)
    {
        // for this cmd, there can be many arguments (arbitrary), but can't be less than 2
        if (args_size < 2)
            return badcommand();
        command_args++;
        return run(command_args);
    }
    else
        return badcommand();
}

int help()
{

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit()
{
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value)
{
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}

int print(char *var)
{
    printf("%s\n", mem_get_value(var));
    return 0;
}

int scheduler(ready rq) // signature should now include PPOLICY, let the scheduler, not exec, decide t=how the queue should be; should also take the list of PCBs
{
    // it would make sense for scheduler to make the readyQueue
    int errCode = 0;

    while (rq.head != NULL)
    {
        pcb *pcb = rq.head;

        while (pcb->index < pcb->p->numOfLines)
        {
            errCode = parseInput(pcb->p->lines[pcb->index]);
            pcb->index++;
        }

        // process finished
        rq.head = pcb->next;

        // cleanup
        for (int i = 0; i < pcb->p->numOfLines; i++)
        {
            free(pcb->p->lines[i]);
        }
        free(pcb->p); // ig it has to be a pointer
        free(pcb);    // by value; same pointer
    }

    return errCode;
}
int source(char *script) // change it so that it uses the newly created DS
{
    // 1)loads the file
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");
    int count = 0;
    if (p == NULL)
    {
        return badcommandFileDoesNotExist();
    }

    while (fgets(line, MAX_USER_INPUT - 1, p) != NULL)
    {
        count++;
    }
    rewind(p);
    program *p1 = malloc(sizeof(program) + count * sizeof(char *));
    p1->numOfLines = count;
    for (int i = 0; i < count; i++)
    {
        fgets(line, MAX_USER_INPUT - 1, p);
        p1->lines[i] = malloc(strlen(line) + 1);
        strcpy(p1->lines[i], line);
    } // program loading is complete
    fclose(p);

    // 2) now create PCB for the program
    pcb *pcb1 = malloc(sizeof(pcb));
    static int pid = 0;
    pcb1->pid = pid++;
    pcb1->index = 0;
    pcb1->p = p1;
    pcb1->next = NULL;
    // 3)add it to the queue (will be the only program init)
    ready rQueue;
    rQueue.head = pcb1;

    return scheduler(rQueue);
}

int exec(char *arg[], int argS) // arg would look like [exec, p1, p2, p3, Policy]
{
    ready rQ;
    int num = argS - 2; // sizeof(processes) / sizeof(char *);
    pcb *l[num];
    char line[MAX_USER_INPUT];

    for (int i = 0; i < num; i++)
    {
        // printf("Opening file index: %d, name: %s\n", i, arg[i + 1]);
        FILE *p = fopen(arg[i + 1], "rt");
        int count = 0;
        if (p == NULL) // if file does not exist, clean up previous pointers then exit
        {
            for (int j = 0; j < i; j++) // for each pcb
            {
                // printf("Cleaning up PCB pointer: %p\n", (void *)l[j]);

                // if (l[j]->p == NULL)
                // {
                //     printf("l[%d]->p is NULL\n", j);
                // }
                // printf("%d\n", l[j]->p->numOfLines);
                for (int x = 0; x < l[j]->p->numOfLines; x++)
                {
                    // printf("%d\n", l[j]->p->numOfLines); // bad naming: p here is program
                    free(l[j]->p->lines[x]);
                }
                printf("%d\n", l[j]->p->numOfLines);

                free(l[j]->p);
                free(l[j]);
            } // have a myFree IMPORTANT
            return badcommandFileDoesNotExist();
        }

        while (fgets(line, MAX_USER_INPUT - 1, p) != NULL)
        {
            count++;
        }

        rewind(p);
        program *p1 = malloc(sizeof(program) + count * sizeof(char *));
        p1->numOfLines = count;
        for (int i = 0; i < count; i++)
        {
            fgets(line, MAX_USER_INPUT - 1, p);
            p1->lines[i] = malloc(strlen(line) + 1);
            strcpy(p1->lines[i], line);
        } // program loading is complete
        fclose(p);

        // 2) now create PCB for the program
        pcb *pcb1 = malloc(sizeof(pcb));
        static int pid = 0;
        pcb1->pid = pid++;
        pcb1->index = 0; // program counter, current line
        pcb1->p = p1;
        pcb1->next = NULL;

        // 3) enqueue (pre)
        l[i] = pcb1;
    }
    rQ.head = l[0];
    for (int i = 0; i < num - 1; i++)
    {
        l[i]->next = l[i + 1];
    }
    return scheduler(rQ);
}

int echo(char *toBeEchoed)
{
    if (toBeEchoed[0] == '$')
    {
        toBeEchoed++; // skip the $
        printf("%s\n", mem_get_value(toBeEchoed));
        return 0;
    }
    else
    {
        printf("%s\n", toBeEchoed);
        return 0;
    }
}

int comp(const void *a, const void *b)
{ // pointer to anything; since a str is alr a pointer, this means arg is a double pointer that must be deferenced
    return strcmp(*(const char **)a, *(const char **)b);
}

int my_ls()
{
    DIR *ds = opendir(".");  // open the current dir
    char *names[1000] = {0}; // init an arr for files&subdirectories
    int len = 0;             // number of files&sd
    struct dirent *name;
    while ((name = readdir(ds)) != NULL)
    {
        names[len] = name->d_name;
        len++;
    }
    closedir(ds);
    qsort(names, len, sizeof(char *), comp); // qsort with cimparison metric defined in comp function
    for (int i = 0; i < len; i++)
    {
        printf("%s\n", names[i]);
    }
    return 0;
}

int my_touch(char *fName)
{
    FILE *ptr = fopen(fName, "wr");
    // alw good practice to check if pointer is null
    if (ptr == NULL)
    {
        return 1;
    }
    fclose(ptr);
    return 0;
}

int my_mkdir(char *dirname)
{

    if (dirname[0] == '$')
    {
        dirname++; // skip the dollar sign
        char *var;
        var = mem_get_value(dirname);
        if (strcmp(var, "Variable does not exist") == 0)
        {
            printf("Bad command: my_mkdir\n");
            return 1;
        }
        if (strchr(var, '_') != NULL) // cannot contain special character, namely the underscore
        {
            printf("Bad command: my_mkdir\n");
            return 1;
        }
        mkdir(var, 0755);
        return 0;
    }
    else
    {
        if (strchr(dirname, '_') != NULL)
        {
            printf("Bad command: my_mkdir\n");
            return 1;
        }
        mkdir(dirname, 0755);
        return 0;
    }
}

int my_cd(char *dirname)
{
    if (chdir(dirname) != 0)
    {
        printf("Bad command: my_cd\n");
        return 1;
    }
    return 0;
}
// create a single subfolder only

int run(char *input[])
{
    int status;
    fflush(stdout);     // flush the buffer
    pid_t pid = fork(); // let the child do the work
    if (pid == 0)
    { // child
        execvp(input[0], input);
    }
    else
    {
        wait(&status);
    }
    return 0;
}