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

int MAX_ARGS_SIZE = 3;

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

int source(char *script) // change it so that it uses the newly created DS
{
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt"); // the program is in a file

    if (p == NULL)
    {
        return badcommandFileDoesNotExist();
    }

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1)
    {
        errCode = parseInput(line); // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p))
        {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
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