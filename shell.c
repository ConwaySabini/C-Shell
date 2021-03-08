/*
* The shell.c file contains a simple shell that can fork a child process and 
* execute a command. The shell also supports executing the most recent command
* with !!. There is also support for redirecting input and output operators,
* concurrency, and simple piping.
*
* @author Sabini Ethan
* @date 1/23/2021
* @version 7.5.0
*/

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
using namespace std;

#define MAX_LINE 80 // The maximum length command

int freeArr(char *args[MAX_LINE])
{
    char *token;
    int i = 0;
    token = args[i];
    while (token != NULL) // free history
    {
        if (token != (char *)NULL)
        {
            free(args[i]);
        }
        i++;
        token = args[i];
    }
    return 1;
}

// method to fork, create child and execute command
int execute(char *args[MAX_LINE], bool should_wait)
{
    int status = 0;
    pid_t pid = fork(); // fork to create child process
    if (pid < 0)
    {
        printf("%s\n", "Error on fork");
        return 0;
    }
    else if (pid == 0) // Child
    {
        int rc = execvp(args[0], args); // execute command with arguments
        if (rc == -1)
        {
            printf("%s\n", "Error on execvp");
        }
        exit(EXIT_SUCCESS);
    }
    else // Parent
    {
        if (should_wait)
        {
            wait(&pid);
        }
        else
        {
            printf("%d\n", pid);
            waitpid(0, &status, WNOHANG);
            should_wait = true;
        }
    }
    return 1;
}

// method for piping commands between child and parent process
int pipe(char *commands[MAX_LINE], int pipes[MAX_LINE], int amper)
{
    int status = 0;
    char *arr[80];
    *arr = (char *)malloc(sizeof(char *) * 80);
    char *arr2[80];
    *arr2 = (char *)malloc(sizeof(char *) * 80);
    int pipeFD[2];
    pipe(pipeFD);
    int p_pid = fork();

    if (p_pid < 0) // Failed
    {
        printf("%s\n", "Error on fork");
        free(*arr);
        free(*arr2);
        return 0;
    }
    else if (p_pid == 0) // Child
    {
        int stdout_cpy = dup(1);
        close(pipeFD[0]);

        int index = 0;
        while (index < pipes[0]) // get strings
        {
            arr[index] = commands[index];
            index++;
        }
        arr[index] = (char *)NULL;

        dup2(pipeFD[1], 1);           // redirect stdout to write to pipe
        int rc = execvp(arr[0], arr); // execute command with arguments
        if (rc == -1)
        {
            printf("%s\n", "Error on execvp");
        }
        dup2(stdout_cpy, 1);
        close(stdout_cpy);
        exit(EXIT_SUCCESS);
    }
    else
    {
        close(pipeFD[1]);
        wait(&p_pid);
    }

    int p2_pid = fork();
    if (p2_pid < 0)
    {
        printf("%s\n", "Error on fork");
        free(*arr);
        free(*arr2);
        return 0;
    }
    else if (p2_pid == 0)
    {
        close(pipeFD[1]);
        close(0);
        int incpy = dup(0);
        dup2(pipeFD[0], 0);

        int index = pipes[0] + 1;
        int arrI = 0; // arr index

        while (commands[index] != NULL)
        {
            arr2[arrI] = commands[index];
            arrI++;
            index++;
        }
        arr2[arrI] = (char *)NULL;
        int rc = execvp(arr2[0], arr2);
        if (rc == -1)
        {
            printf("%s\n", "Error on execvp");
        }
        dup2(incpy, 0);
        close(incpy);
        exit(EXIT_SUCCESS);
    }
    else
    {
        if (amper)
        {
            printf("%d\n", p2_pid);
            waitpid(0, &status, WNOHANG);
        }
    }
    close(pipeFD[0]);
    close(pipeFD[1]);
    wait(NULL);
    wait(NULL);
    free(*arr);
    free(*arr2);
    return 1;
}

int main(void)
{
    const int SIZE = MAX_LINE;
    char *args[SIZE];    // command line command and arguments
    char *history[SIZE]; // stores most recent command
    int file = 0;
    int isHist = false;
    int should_run = 1; // flag to determine when to stop running
    int red_in = 0;
    int red_out = 0;
    int isPipe = 0;
    int pipe_count = 0;
    int pipes[SIZE]; // pipe indicies
    int loop = 0;
    int amper_count = 0;
    int amper = 0;
    char command[MAX_LINE];
    bool should_wait = true;

    *history = (char *)malloc(sizeof(char *) * SIZE);
    *args = (char *)malloc(sizeof(char *) * SIZE);

    while (should_run) // continue to get commands and execute them
    {
        printf("osh>"); // terminal prompt
        fflush(stdout);
        memset(command, 0, sizeof(command));
        memset(args, 0, sizeof(args));

        fgets(command, MAX_LINE, stdin); // get input from the user
        const char delim[2] = " ";       // delimiter for tokenizer
        char *token;
        token = strtok(command, delim); // get first token but modifies string

        if (strcmp(token, "exit\n") == 0) // free memory and exit
        {
            freeArr(history);
            freeArr(args);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(token, "!!\n") == 0) // execute previous command
        {
            if (!isHist)
            {
                printf("%s\n", "No recent command in history");
                continue;
            }
            int l = 0; // index for parsing
            char *tmp = history[l];
            while (tmp != (char *)NULL)
            {
                if (strcmp(tmp, "|") == 0)
                {
                    isPipe = 1;
                    pipe_count = 1;
                    pipes[0] = l;
                }
                else if (strcmp(tmp, "<") == 0)
                {
                    red_in = l;
                    file = l + 1;
                }
                else if (strcmp(tmp, ">") == 0)
                {
                    red_out = l;
                    file = l + 1;
                }
                else if (strcmp(tmp, "&") == 0)
                {
                    should_wait = false;
                    history[l] = (char *)NULL;
                }
                printf("%s", tmp);
                printf("%s", " ");
                l++;
                tmp = history[l];
            }
            printf("%s\n", "");
            if (isPipe || red_in || red_out)
            {
                //do nothing for now
            }
            else // execute normal command from history
            {
                execute(history, should_wait);
            }
        }
        if (strcmp(token, "!!\n") != 0 || isPipe || red_in || red_out)
        {
            if (isPipe) // history pipe
            {
                if (pipe_count == 1)
                {
                    int res = pipe(history, pipes, amper);
                    if (res == 0)
                    {
                        freeArr(history);
                        freeArr(args);
                        exit(EXIT_FAILURE);
                    }
                    isPipe = 0;
                    int cnt = 0;
                    while (cnt < pipe_count) // reset variables
                    {
                        pipes[cnt] = 0;
                        cnt++;
                    }
                    pipe_count = 0;
                    should_wait = true;
                }
                else
                {
                    printf("%s\n", "Shell only supports one pipe at a time");
                }
            }
            else if (red_in) // history redirect in
            {
                int stdin_cpy = dup(STDIN_FILENO);
                int fd = open(history[file], O_RDONLY | O_CREAT, 0666);
                if (!fd)
                {
                    printf("%s\n", "Error on file open");
                    close(fd);
                    continue;
                }
                dup2(fd, STDIN_FILENO); // redirect file to input
                close(fd);
                char ch;
                while ((ch = getc(stdin)) != EOF)
                {
                    putc(ch, stdout);
                }
                dup2(stdin_cpy, STDIN_FILENO); // redirect back
                close(stdin_cpy);
                red_in = false;
                if (!should_wait) // Parent
                {
                    //cout << "pid " << pid << endl;
                    should_wait = true;
                }
            }
            else if (red_out) // history redirect out
            {
                if (!should_wait)
                {
                    //cout << "pid " << pid << endl;
                    should_wait = true;
                }
                int index = red_out;
                char *ptr = history[index];
                while (ptr != (char *)NULL)
                {
                    if (index != file)
                    {
                        history[index] = (char *)NULL;
                        ptr = history[index];
                    }
                    index++;
                }
                int stdout_cpy = dup(STDOUT_FILENO);
                // create if it does not exits or append/write
                int fd = open(history[file], O_WRONLY | O_APPEND | O_CREAT, 0666);
                history[file] = (char *)NULL;
                if (fd == -1)
                {
                    printf("%s\n", "Error on file open");
                    freeArr(history);
                    freeArr(args);
                    exit(EXIT_FAILURE);
                }
                int dup_res = dup2(fd, STDOUT_FILENO); // redirect output to file
                if (!dup_res)
                {
                    printf("%s\n", "Error on redirection");
                    freeArr(history);
                    freeArr(args);
                    exit(EXIT_FAILURE);
                }
                close(fd);
                execute(history, should_wait);
                dup2(stdout_cpy, STDOUT_FILENO); // redirect back
                close(stdout_cpy);
                red_out = false;
            }
            else // Not executing from history
            {
                int count = 0;
                args[count] = token;

                while (token != NULL)
                {
                    if (strcmp(token, " \n") == 0)
                    {
                        token = strtok(NULL, delim);
                        continue;
                    }
                    token = strtok(NULL, delim);
                    count++;
                    args[count] = token;
                }

                int idx = 0;
                token = args[idx];

                while (token != NULL) // remove newline characters
                {
                    token = args[idx];
                    token = strtok(token, "\n");
                    strcat(token, "\0");
                    idx++;
                }
                int i = 0;
                token = history[i];
                while (token != NULL) // free history
                {
                    if (token != (char *)NULL)
                    {
                        free(history[i]);
                    }
                    i++;
                    token = history[i];
                }

                i = 0;
                token = args[i];
                while (token != NULL) // make copy of string and store in history
                {
                    const char *str = token;
                    char *copy = strdup(token);
                    history[i] = copy;
                    if (token != NULL)
                    {
                        if (strcmp(token, ">") == 0)
                        {
                            red_out = i;
                            file = i + 1;
                        }
                        else if (strcmp(token, "<") == 0)
                        {
                            red_in = i;
                            file = i + 1;
                        }
                        else if (strcmp(token, "|") == 0)
                        {
                            isPipe++;
                            pipes[pipe_count] = i;
                            pipe_count++;
                        }
                    }
                    i++;
                    token = args[i];
                }
                if (strcmp(args[count - 1], "&") == 0) // handle concurrency
                {
                    should_wait = false;
                    args[count - 1] = (char *)NULL;
                    amper = count - 1;
                    amper_count++;
                }
                history[count] = (char *)NULL;
                args[count] = (char *)NULL;
                isHist = true;

                if (red_out) // if output redirection
                {
                    if (!should_wait)
                    {
                        //cout << "pid " << pid << endl;
                        //should_wait = true;
                    }
                    int index = red_out;
                    char *ptr = args[index];
                    while (ptr != (char *)NULL)
                    {
                        if (index != file)
                        {
                            args[index] = (char *)NULL;
                            ptr = args[index];
                        }
                        index++;
                    }
                    int stdout_cpy = dup(STDOUT_FILENO);
                    // create or write to file
                    int fd = open(args[file], O_WRONLY | O_APPEND | O_CREAT, 0666);
                    if (fd == -1)
                    {
                        printf("%s\n", "Error on file open");
                        freeArr(history);
                        freeArr(args);
                        exit(EXIT_FAILURE);
                    }
                    int dup_res = dup2(fd, STDOUT_FILENO);
                    if (!dup_res)
                    {
                        printf("%s\n", "Error on redirection");
                        freeArr(history);
                        freeArr(args);
                        exit(EXIT_FAILURE);
                    }
                    close(fd);
                    execute(args, should_wait);
                    dup2(stdout_cpy, STDOUT_FILENO);
                    close(stdout_cpy);
                    red_out = false;
                }
                else if (red_in) // if input redirection
                {
                    int stdin_cpy = dup(STDIN_FILENO);
                    int fd = open(args[file], O_RDONLY | O_CREAT, 0666);
                    if (!fd)
                    {
                        printf("%s\n", "Error on file open");
                        freeArr(history);
                        freeArr(args);
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                    char ch;
                    while ((ch = getc(stdin)) != EOF)
                    {
                        putc(ch, stdout);
                    }
                    int dup_res2 = dup2(stdin_cpy, STDIN_FILENO);
                    close(stdin_cpy);
                    red_in = false;
                    if (!should_wait) // Parent
                    {
                        //cout << "pid " << pid << endl;
                        //should_wait = true;
                    }
                }
                else if (isPipe) // execute pipe
                {
                    if (pipe_count == 1)
                    {
                        int res = pipe(args, pipes, amper);
                        if (res == 0)
                        {
                            freeArr(history);
                            freeArr(args);
                            exit(EXIT_FAILURE);
                        }
                        isPipe = 0;
                        int cnt = 0;
                        while (cnt < pipe_count) // reset variables
                        {
                            pipes[cnt] = 0;
                            cnt++;
                        }
                        pipe_count = 0;
                        should_wait = true;
                    }
                    else
                    {
                        printf("%s\n", "Shell only supports one pipe at a time");
                    }
                }
                else // normal execution
                {
                    int status = 0;
                    pid_t pid = 0;

                    pid = fork(); // fork to create child process

                    if (pid < 0)
                    {
                        printf("%s\n", "Error on fork");
                        freeArr(history);
                        freeArr(args);
                        exit(EXIT_FAILURE);
                    }
                    else if (pid == 0) // Child
                    {
                        fflush(stdout);
                        int rc = execvp(args[0], args); // execute command with arguments
                        if (rc == -1)
                        {
                            printf("%s\n", "Error on exec");
                        }
                        exit(EXIT_SUCCESS);
                    }
                    else // Parent
                    {
                        if (should_wait)
                        {
                            wait(&pid);
                        }
                        else
                        {
                            printf("%d\n", pid);
                            waitpid(0, &status, WNOHANG);
                            should_wait = true;
                        }
                    }
                }
            }
        }

        loop++;
    }
    freeArr(history);
    freeArr(args);
    return 0;
}
