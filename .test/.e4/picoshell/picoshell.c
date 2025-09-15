/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   picoshell.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 10:46:24 by anemet            #+#    #+#             */
/*   Updated: 2025/09/15 17:11:58 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* picoshell()

	Implement the picoshell() function which is executing command pipelines:
	the output of a command is feed as input to the next command

	Key concepts:
	1.) Pipeline: command chain connected by pipes
	2.) Multiple processes: a fork for each command
	3.) Redirection: connecting STDOUT of a cmd to STDIN for the next one
	4.) Sinchronization: wait for all processes to finish
	5.) File descriptors: open and close fd-s at the right moment

	Algorithm:
	1.) for each command (except the last one) create a pipe
	2.) fork() the process for the actual cmd
	3.) child process: configure STDIN/STDOUT and execute the command
	4.) parent process: fd redirect and continuing with the next command
	5.) wait for all processes to finish
*/

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

/* picoshell()
	cmds: Array of string arrays
	e.g.:
	cmds[0] = {"ls", "-l", NULL}
	cmds[1] = {"grep", "txt", NULL}
	cmds[2] = NULL (terminator)

	Return:
		0 - if all cmds executed without error
		1 - if a command or a syscall got an error
*/
int picoshell(char **cmds[])
{
	pid_t pid;
	int pipefd[2];		// actual pipe
	int prev_fd = -1;	// previous file descriptor
	int status;
	int exit_code = 0;
	int i = 0;

	// The main loop - processing each command in the pipeline
	while (cmds[i])
	{
		// create pipe for all commands (except the last one)
		if (cmds[i+1] && pipe(pipefd) == -1)
			return 1;  // if pipe creation fails, return 1

		// fork() process for the current command
		pid = fork();

		// if error in fork(), close fd-s and return 1
		if (pid == -1)
		{
			if (cmds[i + 1])
			{
				close(pipefd[0]);
				close(pipefd[1]);
			}
			return 1;
		}

		// Child process:
		if (pid == 0)
		{
			// configuring STDIN for child:
			// 	if prev_fd: read previous cmd output from there
			if (prev_fd != -1)
			{
				// make prev_fd become the new STDIN
				if (dup2(prev_fd, STDIN_FILENO) == -1)
					exit(1); // exit child with fault code 1 if dup2 fails
				close(prev_fd); // we don't need it, STDIN is enough for us
			}

			// configuring STDOUT for child:
			// 	if there is next cmd, current command should write to pipe
			if (cmds[i + 1])
			{
				close(pipefd[0]); // close pipe read end (we don't need it)
				// make pipefd[1] (pipe write) the new STDOUT
				if (dup2(pipefd[1], STDOUT_FILENO) == -1)
					exit(1); // exit child with fault code 1 if dup2 fails
				close(pipefd[1]); // we don't need it, STDOUT is enough
			}

			// execute the command
			execvp(cmds[i][0], cmds[i]);
			exit(1); // executed only if execvp(0) fails
		}

		// Parent process

		// if prev_fd != -1: close it (prev_fd is used only by the child process)
		if (prev_fd != -1)
			close(prev_fd);

		// current pipe management:
		if (cmds[i + 1])
		{
			close(pipefd[1]);   // close current pipe write
			prev_fd = pipefd[0]; // save current pipe read for the next cmd
		}

		i++;
	}

	// Wait for all child processes to close
	while(wait(&status) != -1)
	{
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			exit_code = 1;
	}

	return exit_code;
}

// Example of picoshell() usage from main:

#include <stdio.h>
#include <string.h>

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
