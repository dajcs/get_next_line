/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sandbox.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/16 10:10:38 by anemet            #+#    #+#             */
/*   Updated: 2025/09/16 15:23:43 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

/* Project sandbox

	Description:
	sandbox() executes a function and determines if it is "good" or "bad".
	The function is bad if: segfault, abort, exit code != 0 or timeout.

	Key concepts:
	- SIGNALS: handle SIGALARM for timeout
	- FORK: execute the function f in a separate process
	- WAITPID: get info about how the process ended
	- ALARM: set a timeout for the function
	- SIGNAL HANDLING: set up signal handlers

	Algorithm:
	- Fork a child process to execute the function
	- Parent: set and alarm and wait with waitpid()
	- Analize how the process terminates (normal, signal, timeout)
	- Return: 1 (good), 0 (bad), -1 (error)
*/


// Global variable for the child process PID
static pid_t child_pid;

// Signal handler for SIGALRM
void alarm_handler(int sig)
{
	/* TIMEOUT HANDLER:
		- runs when alarm() expires
		- doesn't need to do anything special
		- its existance makes waitpid() return with EINTR
	*/
	(void)sig; // Suppress unused parameter warning
}

int sandbox(void (*f)(void), unsigned int timeout, bool verbose)
{
	/* PARAMETERS:
		- f: Function to be tested
		- timeout: timeout in seconds
		- verbose: if true print diagnostic messages

		RETURN:
		- (1): "good" function (exit_code == 0 && no signal interrupt && no timeout)
		- (0): "bad" function (exit_code != 0 || terminated by signal || timeout)
		- (-1): error in the sandbox (failed fork(), ... etc.)
	*/

	struct sigaction sa;
	pid_t pid;
	int status;
	extern int errno;  // Only for IntelliSense; will be ignored if already defined

	/* Set Up SIGALRM Handler:
	- install custom handler for timeout
	- clear the signal mask
	- do not automatically restart syscalls
	*/
	sa.sa_handler = alarm_handler;
	sa.sa_flags = 0;  // No SA_RESTART: we want waitpid to be interrupted
	sigemptyset(&sa.sa_mask);
	sigaction(SIGALRM, &sa, NULL);

	/* sa.sa_flags = 0 => after SIGALRM waitpid() aborted -> return -1, EINTR

            Parent process:
             ┌───────────────┐
             │ waitpid(child)│  <─── blocking here
             └───────┬───────┘
                     │
               [3 sec pass...]
                     │
              Kernel delivers SIGALRM
                     │
             ┌───────▼─────────┐
             │ run alarm_handler│
             └───────┬─────────┘
                     │
                     │ handler returns
                     │
             ┌───────▼─────────────────────────────┐
             │ waitpid() aborted → return -1, EINTR │
             └─────────────────────────────────────┘

	*/

	pid = fork();
	if (pid == -1)
		return -1;  // Error in fork

	/******  CHILD Process ******/
	if (pid == 0)
	{
		/* EXECUTE f in child
		- Call the provided function
		- If it returns normally, exit with code 0
		- If it crashes (segfault/abort), the kernel will send a signal
		*/
		f();
		exit(0); // Function f finished normally
	}

	/******* PARENT process *******/
	child_pid = pid;

	/* Establishing TIMEOUT
	- alarm() sends SIGALRM after timeout seconds
	- this interrupts waitpid() if function f is still running
	*/
	alarm(timeout);

	/* Wait for CHILD process to finish
	- waitpid() can return for various reasons:
		- the Child terminates normally (exit)
		- the Child is terminated by a signal
		- waitpid() is interrupted by SIGALRM (timeout)
	*/
	if (waitpid(pid, &status, 0) == -1)
	{
		if (errno == EINTR) // interrupted by SIGALRM
		{
			/* TIMEOUT detected:
			- waitpid() has been interrupted by alarm
			- the child is probably still running
			- Kill it with SIGKILL and record its state
			*/
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0); // Reap zombie process

			if (verbose)
				printf("Bad function: timed out after %d seconds\n", timeout);
			return 0;
		}
		return -1; // errno != EINTR, => sandbox error
	}

	/* ANALYZE How the Process TERMINATED */
	if (WIFEXITED(status))
	{
		/* NORMAL Termination (exit):
		- The process called exit() or returned from main
		- Check the exit code
		*/
		if (WEXITSTATUS(status) == 0)
		{
			if (verbose)
				printf("Nice function!\n");
			return 1; // "good" function
		}
		else
		{
			if (verbose)
				printf("Bad function: exited with code %d\n", WEXITSTATUS(status));
			return 0; // "bad" function
		}
	}

	if (WIFSIGNALED(status))
	{
		/* TERMINATION by SIGNAL
		- the process was killed by a signal (segfault, abort, etc.)
		- get signal number for diagnostics
		*/
		int sig = WTERMSIG(status);
		if (verbose)
			printf("Bad function: %s\n", strsignal(sig));
		return 0; // "bad" function
	}

	return -1; // Unrecognized status
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
