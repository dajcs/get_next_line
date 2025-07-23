/* ************************************************************************** */
/*                                                                            */
/*   filter.c                                                                 */
/*   Replaces every occurrence of the pattern given as argv[1] with '*'s.     */
/*                                                                            */
/* ************************************************************************** */

#define _GNU_SOURCE  // for memmem()

#include <unistd.h>      // read, write
#include <stdlib.h>      // malloc, realloc, free
#include <string.h>      // strlen, memmem
#include <errno.h>       // errno
#include <stdio.h>       // perror

#ifndef BUFFER_SIZE
# define BUFFER_SIZE 42
#endif


void ft_filter(char *buffer, char *pattern)
{
	int i;
	int p_len;
	int j;
	int k;

	i = 0;
	p_len = strlen(pattern);
	while (buffer[i])
	{
		j = 0;
		while (pattern[j] && (buffer[i + j] == pattern[j]))
			j++;
		if (j == p_len)
		{
			k = 0;
			while (k < p_len)
			{
				write(1, "*", 1);
				k++;
			}
			i += p_len;
			continue;
		}
		else
		{
			write(1, &buffer[i], 1);
			i++;
		}
	}
}


int main(int argc, char **argv)
{
	char temp[BUFFER_SIZE];
	char *res;
	char *buffer;
	int total_read;
	ssize_t bytes;

	if (argc != 2 || argv[1] == NULL || argv[1][0] == '\0')
	{
		printf("argv[1] fault\n");
		return 1;
	}

	res = NULL;
	total_read = 0;
	while((bytes = read(STDIN_FILENO, temp, BUFFER_SIZE)) > 0)
	{
		buffer = realloc(res, (total_read + bytes + 1));
		if (!buffer)
		{
			free(res);
			perror("realloc");
			return 1;
		}
		res = buffer;
		// [res ... res + total_read) already filled 
		memmove((res + total_read), temp, bytes);
		total_read = total_read + bytes;
		res[total_read] = '\0';
	}
	if (bytes < 0)
	{
		perror("read");
		free(res);
		return 1;
	}
	if (!res)
		return 0;
	ft_filter(res, argv[1]);
	free(res);
	return 0;
}
