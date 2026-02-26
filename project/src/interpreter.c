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

int schedulerIsRunning = 0; // if 1, the exec will add more programs to the queue
int MAX_ARGS_SIZE = 6;
ready rq;
int background = 0;

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
int scheduler(char *policy, pcb *pcbList[], int length);
int exec(char *arg[], int argS);
int compSJF(const void *a, const void *b);
void myFree(pcb *pcb);

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
        if (args_size < 3 || args_size > 6)
        {
            return badcommand();
        }
        // if (*command_args[args_size - 1] != '#' && ((command_args[args_size - 1], "FCFS") != 0 && strcmp(command_args[args_size - 1], "SJF") != 0 && strcmp(command_args[args_size - 1], "RR") != 0 && strcmp(command_args[args_size - 1], "AGING") != 0 && strcmp(command_args[args_size - 1], "RR30") != 0)) // for 1.2.2 we just check FCFS
        // {
        //     return badcommand();
        // }
        if (*command_args[args_size - 1] != '#' &&
            (strcmp(command_args[args_size - 1], "FCFS") != 0 &&
             strcmp(command_args[args_size - 1], "SJF") != 0 &&
             strcmp(command_args[args_size - 1], "RR") != 0 &&
             strcmp(command_args[args_size - 1], "AGING") != 0 &&
             strcmp(command_args[args_size - 1], "RR30") != 0))
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

int scheduler(char *policy, pcb *l[], int length) // signature should now include PPOLICY, let the scheduler, not exec, decide t=how the queue should be; should also take the list of PCBs
{
    schedulerIsRunning = 1;
    // it would make sense for scheduler to make the readyQueue
    int errCode = 0;

    if (strcmp(policy, "SJF") == 0 || strcmp(policy, "AGING") == 0)
    {
        if (background)
        {
            qsort(++l, length - 1, sizeof(pcb *), compSJF);
            l--;
        }
        else
            qsort(l, length, sizeof(pcb *), compSJF);
    }
    background = 0; // reset after sorting decision is made

    // ready rq = {l[0]}; update during task 1.2.5: make a global readyQ
    rq.head = l[0];
    for (int i = 0; i < length; i++)
    {

        if (i == length - 1)
        {
            l[i]->next = NULL;
        }
        else
        {
            l[i]->next = (struct pcb *)l[i + 1];
        }
    }

    if (strcmp(policy, "FCFS") == 0 || strcmp(policy, "SJF") == 0) // one loope for nonpreemptive, then another for preemptive policies
    {

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

        // return errCode;
    }

    else if (strcmp(policy, "RR") == 0 || strcmp(policy, "RR30") == 0)
    { // preemptive
        int lim = 2;
        if (strcmp(policy, "RR30") == 0)
        {
            lim = 30;
        }
        while (rq.head != NULL) // as long as queue not empty (program not finished)
        {
            pcb *prog = rq.head;
            for (int i = 0; i < lim; i++) // e
            {
                if (prog->index < prog->p->numOfLines) // if still lines to exec
                {
                    errCode = parseInput(prog->p->lines[prog->index]);
                    prog->index++;
                    if (i == lim - 1)
                    {
                        rq.head = prog->next;
                        enqueue(&rq, prog); // remove it from head and stick it to the tail
                    }
                }
                else
                {
                    rq.head = prog->next; // remove
                    myFree(prog);
                    break;
                }
            }
        }
        // return errCode;
    }

    else if (strcmp(policy, "AGING") == 0)
    {
        while (rq.head != NULL)
        {
            pcb *pr = rq.head;
            // execute 1 ins
            errCode = parseInput(pr->p->lines[pr->index]);
            pr->index++;
            if (pr->index >= pr->p->numOfLines)
            {
                // ce programme se termine
                rq.head = pr->next; // remove
                myFree(pr);
            }
            else
            {
                rq.head = pr->next;
                pr->next = NULL; // need to consider case when queue has only 1 program
                pcb *temp = rq.head;
                while (temp != NULL)
                {
                    if (temp->score > 0)
                        temp->score--;
                    temp = temp->next;
                }
                // rq.head = rq.head->next; put removal of current head in enqueueAging
                enqueueAging(&rq, pr);
            }
        }
        // return errCode;
    }
    schedulerIsRunning = 0;
    return errCode;
}

void myFree(pcb *pcb)
{
    for (int x = 0; x < pcb->p->numOfLines; x++)
    {
        free(pcb->p->lines[x]);
    }
    free(pcb->p);
    free(pcb);
}
int exec(char *arg[], int argS) // arg would look like [exec, p1, p2, p3, Policy]
{

    static int pid = 0;
    background = 0;
    if (strcmp(arg[argS - 1], "#") == 0)
    {
        background = 1;
        argS--; // remove # so policy is now arg[argS-1]
    }

    int num = argS - 2; // sizeof(processes) / sizeof(char *);
    pcb *l[num + 1];    // BC batch process (p0)
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
                myFree(l[j]);
            }
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
        pcb1->pid = pid++;
        pcb1->index = 0; // program counter, current line
        pcb1->p = p1;
        pcb1->score = pcb1->p->numOfLines;
        pcb1->next = NULL;

        // 3) enqueue (pre)
        l[i] = pcb1;
    } // l now is a list of PCBs

    if (background)
    {
        char lineBuf[MAX_USER_INPUT];
        char *tempLines[1000];
        int count = 0;

        while (fgets(lineBuf, MAX_USER_INPUT - 1, stdin) != NULL)
        {
            tempLines[count++] = strdup(lineBuf); // save each line
        }
        program *p0 = malloc(sizeof(program) + count * sizeof(char *));
        p0->numOfLines = count;
        for (int i = 0; i < count; i++)
            p0->lines[i] = tempLines[i]; // already strdup'd

        pcb *pcb0 = malloc(sizeof(pcb));
        pcb0->pid = pid++;
        pcb0->index = 0;
        pcb0->p = p0;
        pcb0->score = count;
        pcb0->next = NULL;

        // shift l right and put pcb0 first
        for (int i = num; i > 0; i--)
            l[i] = l[i - 1];
        l[0] = pcb0;
        num++;
    }

    if (schedulerIsRunning == 1)
    {
        // add the new programs to the queue, make rq visible. and no need to run the scheduler again (it's alr
        // running that's why you are brought here when seeing the 2nd exec)
        if (strcmp(arg[argS - 1], "RR") == 0 || strcmp(arg[argS - 1], "RR30") == 0 ||
            strcmp(arg[argS - 1], "FCFS") == 0)
        {
            for (int k = 0; k < num; k++)
            {
                enqueue(&rq, l[k]);
            }
        }

        if (strcmp(arg[argS - 1], "AGING") == 0 || strcmp(arg[argS - 1], "SJF") == 0)
        {
            // Insert in SJF order but don't preempt the currently running head
            for (int k = 0; k < num; k++)
            {
                pcb *p = l[k];
                // Find position after head, maintaining SJF order
                pcb *temp = rq.head;
                while (temp->next != NULL && temp->next->score <= p->score)
                {
                    temp = temp->next;
                }
                p->next = temp->next;
                temp->next = p;
            }
        }
        return 0; // success, back the the caller which is scheduler
    }
    schedulerIsRunning = 0; // reset
    return scheduler(arg[argS - 1], l, num);
}

int compSJF(const void *a, const void *b)
{
    pcb *pcbA = *(pcb **)a;
    pcb *pcbB = *(pcb **)b;
    return pcbA->p->numOfLines - pcbB->p->numOfLines;
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

int source(char *script)
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

    pcb *l[] = {pcb1};

    return scheduler("FCFS", l, 1);
}