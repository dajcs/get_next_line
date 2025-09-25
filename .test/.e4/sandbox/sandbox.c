/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sandbox.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/25 14:00:20 by anemet            #+#    #+#             */
/*   Updated: 2025/09/25 15:35:21 by anemet           ###   ########.fr       */
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
"Nice function!"
"Bad function: exited with code <exit_code>"
"Bad function: <signal description>"
"Bad function: timed out after <timeout> seconds"

You must not leak processes (even in zombie state, this will be checked using wait).

We will test your code with very bad functions.

*/

#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

void alarm_handler(int sig)
{
	(void)sig;
}

int sandbox(void (*f)(void), unsigned int timeout, bool verbose)
{
	pid_t pid;
	int	status;

	// man 7 signal

	/* man 2 sigaction
           struct sigaction {
               void     (*sa_handler)(int);		--> a void function that expects (int sig)
               void     (*sa_sigaction)(int, siginfo_t *, void *);  X enough to set sa_handler
               sigset_t   sa_mask;    			--> set it with sigemptyset(&sa.sa_mask)
               int        sa_flags;   			--> int, set it to 0
               void     (*sa_restorer)(void);
           };
	*/

	/* man 3 sigaction
           static void handler(int signum)
           {
               // Take appropriate actions for signal delivery
           }

		   int main()
           {
               struct sigaction sa;

               sa.sa_handler = handler;
               sigemptyset(&sa.sa_mask);
               sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler
               if (sigaction(SIGINT, &sa, NULL) == -1)
                   // Handle error ;
               // Further code
           }
	*/
	struct sigaction sa;
	sa.sa_handler = alarm_handler;
	sigemptyset(&sa.sa_mask); // sets sa.sa_mask to exclude additional signals from the set
	sa.sa_flags = 0;  // no SA_RESTART; we need waitpid() interrupted by SIGALRM
	if (sigaction(SIGALRM, &sa, NULL) == -1) // change the action on receipt of SIGALRM to sa
		return -1;

	if (!f)
		return -1;

	pid = fork();
	if (pid == -1)
		return -1;

	/* Child Process */
	if (pid == 0)
	{
		f();   // execute function, if crashes kernel sends signal
		exit(0); // f() exited without signal
	}

	/* Parent process */

	// check timeout
	alarm(timeout); // kernel sends SIGALRM after timeout
					// interrupting waitpid() if f() still running

	if (waitpid(pid, &status, 0) == -1) // sets status
										// returns -1 on some kind of error, e.g. interrupt
	{
		// bash cmd: `errno -l | grep -i interrupt`
		// EINTR 4 Interrupted system call
		if (errno == EINTR)		//interrupted by kernel SIGALRM
		{
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0); // reap zombi process
		}
		if (verbose)
			printf("Bad function: timed out after %d seconds\n", timeout);
		return 0;
	}

	// man sys_wait.h

	// check "normal" (not signal) exit
	if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status) == 0)
		{
			if (verbose)
				printf("Nice function!\n");
			return 1;
		}
		else
		{
			if (verbose)
				printf("Bad function: exited with code %d\n", WEXITSTATUS(status));
			return 0;
		}
	}

	// check signalled exit
	if (WIFSIGNALED(status))
	{
		int sig = WTERMSIG(status);
		if (verbose)
			printf("Bad function: %s\n", strsignal(sig));
		return 0;
	}

	return -1;  // sandbox error
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
	result = sandbox(bad_function_timeout, 2, true);
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
