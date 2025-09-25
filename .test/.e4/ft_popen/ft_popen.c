/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_popen.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/25 20:28:15 by anemet            #+#    #+#             */
/*   Updated: 2025/09/25 21:22:54 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*

Assignment name  : ft_popen
Expected files   : ft_popen.c
Allowed functions: pipe, fork, dup2, execvp, close, exit
--------------------------------------------------------------------------------------

Write the following function:

int ft_popen(const char *file, char *const argv[], char type);

The function must launch the executable file with the arguments argv (using execvp).
If type is 'r' the function must return a file descriptor connected to the output of the command.
If type is 'w' the function must return a file descriptor connected to the input of the command.
In case of error or invalid parameter the function must return -1.

For example, the function could be used like that:

int main()
{
    int  fd;
    char *line;

    fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
    while ((line = get_next_line(fd)))
        ft_putstr(line);
    return (0);
}


int     main() {
        int     fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
        dup2(fd, 0);
        fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');
        char    *line;
        while ((line = get_next_line(fd)))
                printf("%s", line);
}


Hints:
Do not leak file descriptors!
This exercise is inspired by the libc's popen().

Please type 'test' to test code, 'next' for next or 'exit' for exit.

*/

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>



int ft_popen(const char *file, char *const argv[], char type)
{
	pid_t pid;
	int p[2];

	if (!file || !argv || (type != 'r' && type != 'w'))
		return -1;

	if (pipe(p) == -1)
		return -1;

	pid = fork();
	if (pid == -1)
	{
		close(p[0]);
		close(p[1]);
		return -1;
	}

	/* Child process */
	if (pid == 0)
	{
		// type == 'r': we're going to read from the process in parent
		//				child process writes to pipe
		if (type == 'r')
		{
			// pipe write = STDOUT
			close(p[0]); // close pipe read
			if (dup2(p[1], STDOUT_FILENO) == -1)
				return -1;
			close(p[1]);
		}
		// type == 'w': we're going to write to the process in parent
		//				child process reads from the pipe
		if (type == 'w')
		{
			// pipe read = STDIN
			close(p[1]); // close pipe write
			if (dup2(p[0], STDIN_FILENO) == -1)
				return -1;
			close(p[0]);
		}

		// execute process
		execvp(file, argv);
		exit(1); // reached only on execvp error
	}

	/* Parent process */
	else
	{
		// type == 'r': we're returning the pipe read end
		if (type == 'r')
		{
			close(p[1]);
			return p[0];
		}
		// type == 'w': we're returning the pipe write end
		if (type == 'w')
		{
			close(p[0]);
			return p[1];
		}
	}
	return 1; // shouldn't be reached
}

/*
int     main()
{
	int     fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
	dup2(fd, 0);
	fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');

	// man readline

	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	FILE *stream = fdopen(fd, "r");
	if (stream == NULL)
	{
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	while ((nread = getline(&line, &len, stream)) != -1)
		printf("%s", line);

	free(line);
	fclose(stream);
	exit(EXIT_SUCCESS);
}
 */
