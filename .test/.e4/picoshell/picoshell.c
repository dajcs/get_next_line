/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   picoshell.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 15:49:38 by anemet            #+#    #+#             */
/*   Updated: 2025/09/23 17:03:35 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
Assignment name:	picoshell
Expected files:		picoshell.c
Allowed functions:	close, fork, wait, exit, execvp, dup2, pipe
___________________________________________________________________

Write the following function:

int    picoshell(char **cmds[]);

The goal of this function is to execute a pipeline. It must execute each
commands of cmds and connect the output of one to the input of the
next command (just like a shell).
e
Cmds contains a null-terminated list of valid commands. Each rows
of cmds are an argv array directly usable for a call to execvp. The first
arguments of each command is the command name or path and can be passed
directly as the first argument of execvp.

If any error occur, The function must return 1 (you must of course
close all the open fds before). otherwise the function must wait all child
processes and return 0. You will find in this directory a file main.c which
contain something to help you test your function.


Examples:
./picoshell /bin/ls "|" /usr/bin/grep picoshell
picoshell
./picoshell echo 'squalala' "|" cat "|" sed 's/a/b/g'
squblblb
*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>

int picoshell(char **cmds[])
{
	int prev_fd = -1;
	int pipefd[2];
	int exit_code = 0;
	int status;
	pid_t pid;
	int i = 0;

	if (!cmds || !cmds[0])
		return 1;

	while (cmds[i])
	{
		if (cmds[i + 1])
		{
			if (pipe(pipefd) == -1)
				return 1;
		}
		pid = fork();
		if (pid == -1)
		{
			if (cmds[i + 1])
			{
				close(pipefd[0]);
				close(pipefd[1]);
			}
			return 1;
		}
		// child process
		if (pid == 0)
		{
			// if prev cmd, dup2 prev_fd = STDIN
			if (prev_fd != -1)
			{
				if (dup2(prev_fd, STDIN_FILENO) == -1)
					exit(1);
				close(prev_fd);
			}
			// if next cmd dup2 pipe write = STDOUT
			// 		and close current pipe read
			if (cmds[i + 1])
			{
				close(pipefd[0]);
				if (dup2(pipefd[1], STDOUT_FILENO) == -1)
					exit(1);
				close(pipefd[1]);
			}
			// execute cmd
			execvp(cmds[i][0], cmds[i]);
			exit(1); // normally not reached
		}
		// parent process
		else
		{
			if (prev_fd != -1)
				close(prev_fd); // prev_fd used by child only

			// if next cmd, save pipe read = prev_fd
			if (cmds[i + 1])
			{
				close(pipefd[1]); // close current pipe write
				prev_fd = pipefd[0];
			}
		}
		i++; // next cmd
	}
	// wait for processes to end
	while(wait(&status) != -1)
	{
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			exit_code = 1;
	}
	return exit_code;
}

int main()
{

	char *cmd1[] = {"ls", NULL};
	char *cmd2[] = {"grep", "c", NULL};
	char **cmds[] = {cmd1, cmd2, NULL};

	// Test 1
    // char *cmd1[] = {"echo", "Hello World", NULL};
    // char **cmds[] = {cmd1, NULL};

	// Test 2
    // char *cmd1[] = {"echo", "Pipeline Test", NULL};
    // char *cmd2[] = {"cat", NULL};
    // char **cmds[] = {cmd1, cmd2, NULL};

	// Test 3
    // char *cmd1[] = {"echo", "word1 word2 word3", NULL};
    // char *cmd2[] = {"cat", NULL};
    // char *cmd3[] = {"wc", "-w", NULL};
    // char **cmds[] = {cmd1, cmd2, cmd3, NULL};

	// Test empty cmds
    // char **cmds[] = {NULL};

	// int exit_code = picoshell(cmds);
	// printf("\nexit_code: %d\n\n", exit_code);
}
/*
static int count_cmds(int argc, char **argv)
{
    int count = 1;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "|") == 0)
            count++;
    }
    return count;
}

int main(int argc, char **argv)
{
    if (argc < 2)
		// return (expr1, expr2) -> expr1 side effect: print usage; return expr2 (1)
        return (fprintf(stderr, "Usage: %s cmd1 [args] | cmd2 [args] ...\n", argv[0]), 1);

    int cmd_count = count_cmds(argc, argv);
    char ***cmds = calloc(cmd_count + 1, sizeof(char **));
    if (!cmds)
		// return (expr1, expr2) -> expr1 side effect: print calloc error; return expr2 (1)
        return (perror("calloc"), 1);

    // argument parsing and constructing cmds array
    int i = 1, j = 0;
    while (i < argc)
    {
        int len = 0;
        while (i + len < argc && strcmp(argv[i + len], "|") != 0)
            len++;

        cmds[j] = calloc(len + 1, sizeof(char *));
        if (!cmds[j])
            return (perror("calloc"), 1);

        for (int k = 0; k < len; k++)
            cmds[j][k] = argv[i + k];
        cmds[j][len] = NULL;

        i += len + 1;  // skip "|"
        j++;
    }
    cmds[cmd_count] = NULL;

    int ret = picoshell(cmds);

    // free memory
    for (int i = 0; cmds[i]; i++)
        free(cmds[i]);
    free(cmds);

    return ret;
}
 */


/* The diagram of "ls | grep txt | wc -l" pipeline

	Command 1: ls
	-------------
	child 1:	STDIN = Terminal, STDOUT = pipe1[1], STDERR = terminal
	parent:		                , close(pipe1[1]), prev_fd = pipe1[0]

	pipe1: 		ls -> pipe1[1] (child) -> pipe1 buffer -> pipe1[0] = prev_fd (parent) ->...

	Command 2: grep txt
	-------------------
	child 2:	STDIN = pipe1[0], STDOUT = pipe2[1], STDERR = terminal
	parent:		close(pipe1[0]) , close(pipe2[1]), prev_fd = pipe2[0]

	pipe2: 		grep txt -> pipe2[1] (child) -> pipe2 buffer -> pipe2[0] = prev_fd (parent) -> ...

	Command 3: wc -l
	----------------
	child 3:	STDIN = pipe2[0], STDOUT = Terminal, STDERR = terminal
	parent:		close(pipe2[0]) ,                  ,

	Data flow: ls -> pipe1 -> grep txt -> pipe2 -> wc -l -> terminal

*/


/* Common Errors and How to Avoid Them

	Deadlock due to open fd-s:
	- if we're not closing pipe write end in parent, next cmd never gets EOF
	- Result: the pipeline is hanging indefinitely

	Broken pipe:
	- if we're closing fd-s in incorrect order (before duplicate)
	- Result: the process gets signal SIGPIPE and terminates

	Zombie processes:
	- if we don't wait() for all the child processes
	- Result: zombi processes in the system

	FD leaks:
	- if we're not closing unused fd-s
	- Result: it can exhaust the system FD table

*/


/* Key points at the exam

	Order of the operations:
	- create pipe BEFORE fork (pipe fd-s reachable in parent and child as well)
	- set up redirections in the child
	- close appropriate fd-s in parent and child

	Managing prev_fd:
	- it represents the "output" of the previous command
	- becomes the "input" of the current command
	- must be closed after use

	Last command special case:
	- it doesn't need an output pipe
	- its STDOUT goes to the terminal
	- check cmds[i + 1] before creating a pipe

	Error handling:
	- check return values of pipe(), fork(), dup2()
	- close fd-s on error before return 1
	- return 1 if any command fails

	Synchronization:
	- wait() in loop until no children remain
	- check exit codes of all processes
	- a signle failed process makes the whole pipeline fail

*/
