/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sandbox.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 10:10:38 by anemet            #+#    #+#             */
/*   Updated: 2025/09/22 11:36:41 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*

Assignment name  : sandbox
Expected files   : sandbox.c
Allowed functions: fork, waitpid, exit, alarm, sigaction, kill, printf, strsignal,
errno, sigaddset, sigemptyset, sigfillset, sigdelset, sigismember
--------------------------------------------------------------------------------------

Write the following function:

#include <stdbool.h>
int sandbox(void (*f)(void), unsigned int timeout, bool verbose);

This function must test if the function f is a nice function or a bad function, you
will return 1 if f is nice, 0 if f is bad or -1 in case of an error in your function.

A function is considered bad if it is terminated or stopped by a signal (segfault, abort...),
if it exit with any other exit code than 0 or if it times out.

If verbose is true, you must write the appropriate message among the following:
"Nice function!\n"
"Bad function: exited with code <exit_code>\n"
"Bad function: <signal description>\n"
"Bad function: timed out after <timeout> seconds\n"

You must not leak processes (even in zombie state, this will be checked using wait).

We will test your code with very bad functions.

*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // exit()
#include <unistd.h>  // fork(), alarm(), kill()
#include <sys/wait.h> // waitpid(), WIFEXITED(), WIFSIGNALED(),...

// global var for child pid
static pid_t child_pid;

// alarm handler for SIGALRM
void alarm_handler(int sig)
{
	(void)sig;
}

int sandbox(void (*f)(void), unsigned int timeout, bool verbose)
{
	struct sigaction sa;
	pid_t pid;
	int status;

	sa.sa_handler = alarm_handler;
	sa.sa_flags = 0; // no SA_RESTART, we want waitpid interrupted
	sigemptyset(&sa.sa_mask); // all signals excluded from the set
	sigaction(SIGALRM, &sa, NULL); // change the action on receipt of SIGALRM to sa

	if(!f)
		return -1;

	pid = fork();
	if (pid == -1)
		return -1;

	/* Child Process */
	if (pid == 0)
	{
		f();     // if crashes kernel sends signal
		exit(0); // f() exited normally
	}

	/* Parent Process */
	child_pid = pid;

	alarm(timeout); // sends SIGALRM after timetout, interrupting waitpid() if f() still running

	if (waitpid(pid, &status, 0) == -1)  // on error -1 is returned
	{
		if (errno == EINTR)  // interrupted by system call SIGALRM  ( `errno -l`)
		{
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0); // reap zombi process
		}
		if (verbose)
			printf("Bad function: timeout after %d seconds\n", timeout);
		return 0;
	}

	if (WIFEXITED(status))   // normal termination exit
	{
		if (WEXITSTATUS(status) == 0)  // nice function
		{
			if (verbose)
				printf("Nice function!\n");
			return 1;
		}
		else  // bad function
		{
			if (verbose)
				printf("Bad function, exited with code %d\n", WEXITSTATUS(status));
			return 0;
		}
	}

	if (WIFSIGNALED(status)) // process killed by SIGNAL
	{
		int sig = WTERMSIG(status);
		if (verbose)
			printf("Bad function: %s\n", strsignal(sig));
		return 0; // bad function
	}
	return -1; // sandbox fail
}

/* EXAMPLES of Functions to be tested */

void nice_function(void)
{
	// this function doesn't do anything bad
	return;
}

void bad_function_exit_code(void)
{
	// This function exits with error code 42
	exit(42);
}

void bad_function_segfault(void)
{
	// this function is causing a segmentation fault
	int *ptr = NULL;
	*ptr = 42;
}

void bad_function_timeout(void)
{
	// this function never terminates
	while (1)
	{

	}
}

void bad_function_abort(void)
{
	// this function calls abort()
	abort();
}

void bad_function_divide_by_zero(void)
{
	int x = 1;
	int y = 0;
	volatile int result = x / y; // Should cause SIGFPE - Floating Point Exception
	(void)result;
}

void very_bad_function_stack_overflow(void)
{
	// Cause stack overflow - a very bad function
	char huge_buffer[100000];
	// use volatile to prevent optimization
	volatile int x = 1;
	if (x)
		very_bad_function_stack_overflow();
	(void)huge_buffer;
}

/* Example of Use */

int main()
{
	int result;

	printf("Testing nice_function():\n");
	result = sandbox(nice_function, 5, true);
	printf("Result: %d\n\n", result); // Expecting 1

	printf("Testing bad_function_exit_code():\n");
	result = sandbox(bad_function_exit_code, 5, true);
	printf("Result: %d\n\n", result); // Expecting 42

	printf("Testing bad_function_segfault():\n");
	result = sandbox(bad_function_segfault, 5, true);
	printf("Result: %d\n\n", result); // Expecting 0

	printf("Testing bad_function_timeout():\n");
	result = sandbox(bad_function_timeout, 5, true);
	printf("Result: %d\n\n", result); // Expecting 0

	printf("Testing bad_function_abort():\n");
	result = sandbox(bad_function_abort, 5, true);
	printf("Result: %d\n\n", result); // Expecting 0

	printf("Testing bad_function_divide_by_zero():\n");
	result = sandbox(bad_function_divide_by_zero, 5, true);
	printf("Result: %d\n\n", result); // Expecting 0

	printf("Testing very_bad_function_stack_overflow():\n");
	result = sandbox(very_bad_function_stack_overflow, 5, true);
	printf("Result: %d\n\n", result); // Expecting 0

	return 0;
}


/* Key Points for the Examen

1.	SIGNAL SETUP:
	- use sigaction() instead of signal() (more portable)
	- do not use SA_RESTART: we want waitpid() to be interrupted
	- the handler can be empty,; it just needs to exist

2.	TIMEOUT DETECTION
	- alarm() + waitpid() interrupted by SIGALRM
	- Check errno == EINTR
	- Kill the child process and reap the zombie

3.	STATUS ANALYSIS:
	- Use macros WIFEXITED, WIFSIGNALED, etc
	- do not assume a specific status format
	- handle all possible cases

4.	LEAK PREVENTION:
	- Always call waitpid() after kill()
	- do not leave zombie processes
	- Ensure all code paths clean up processes

5.	ROBUSTNESS
	- Handle errors from fork(), waitpid(), etc.
	- Very bad functions can do unexpected things
	- The sandbox mst be more robust than the functions it tests



*/
