/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_popen.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 15:07:10 by anemet            #+#    #+#             */
/*   Updated: 2025/09/24 15:51:54 by anemet           ###   ########.fr       */
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

int	main() {
	int	fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
	dup2(fd, 0);
	fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');
	char	*line;
	while ((line = get_next_line(fd)))
		printf("%s", line);
}

Hints:
Do not leak file descriptors!
This exercise is inspired by the libc's popen().

*/

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

int ft_popen(const char *file, char *const argv[], char type)
{
	int p[2]; // pipe fd-s
	pid_t pid;

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
	/* child process */
	if (pid == 0)
	{
		// type = 'r': we want to read from child process
		//				pipe write = STDOUT
		if (type == 'r')
		{
			close(p[0]); // close pipe read
			if (dup2(p[1], STDOUT_FILENO) == -1)
				return -1;
			close(p[1]); // STDOUT is writing to the pipe
		}

		// type = 'w': we want to write to the child process
		//				pipe read = STDIN
		if (type == 'w')
		{
			close(p[1]); // close pipe write
			if (dup2(p[0], STDIN_FILENO) == -1)
				return -1;
			close(p[0]); // pipe read is coming through STDIN
		}
		execvp(file, argv);
		exit(0); // should not be reached
	}
	/* parent process */
	else
	{
		// type = 'r': we want to read from child process
		//				=> return pipe read end
		close(p[1]);  // child is writing to the pipe
		return (p[0]);

		// type = 'w': we want to write to the child process
		//				=> return pipe write end
		close(p[0]);  // child is reading from the pipe
		return (p[1]);
	}
}

int	main()
{
	int	fd = ft_popen("ls", (char *const []){"ls", NULL}, 'r');
	dup2(fd, 0);
	fd = ft_popen("grep", (char *const []){"grep", "c", NULL}, 'r');

	FILE *stream = fdopen(fd, "r");

	char	*line = NULL;  // must be NULL because getline is using realloc()
	size_t len;
	ssize_t read;

	while ((read = getline(&line, &len, stream)) != -1)
		printf("%s", line);

	free(line);
	fclose(stream); // this closes fd as well
}

