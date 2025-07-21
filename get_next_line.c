/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get_next_line.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 11:30:12 by anemet            #+#    #+#             */
/*   Updated: 2025/07/21 15:51:29 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "get_next_line.h"

static char	*gnl_free(char *buf)
{
	free(buf);
	return (NULL);
}

// fill stash until '\n' in stash or EOF
// read() returns nr of bytes read
// new stash = old stash + buf // gnl_strjoin frees old stash
static char	*gnl_read(int fd, char *stash)
{
	char	*buf;
	int		rd;

	buf = malloc(BUFFER_SIZE + 1);
	if (!buf)
		return (NULL);
	rd = 1;
	while (!gnl_strchr(stash, '\n') && rd > 0)
	{
		rd = read(fd, buf, BUFFER_SIZE);
		if (rd < 0)
			return (gnl_free(buf));
		buf[rd] = '\0';
		stash = gnl_strjoin(stash, buf);
		if (!stash)
			return (gnl_free(buf));
	}
	free(buf);
	return (stash);
}

// throws away the last served line and returns the tail (if any)
static char	*gnl_new_stash(char *stash)
{
	size_t	i;
	char	*new;

	i = 0;
	while (stash[i] && stash[i] != '\n')
		i++;
	if (!stash[i])
	{
		free(stash);
		return (0);
	}
	new = gnl_substr(stash, i + 1, gnl_strlen(stash + i + 1));
	free(stash);
	return (new);
}

// returns a copy of the first complete line (including '\n' if present)
static char	*gnl_get_line(char *stash)
{
	size_t	i;
	char	*line;

	if (!stash || !stash[0])
		return (NULL);
	i = 0;
	while (stash[i] && stash[i] != '\n')
		i++;
	if (stash[i] == '\n')
		i++;
	line = gnl_substr(stash, 0, i);
	return (line);
}

// stash[fd] keeps whatever was read from file, but not yet returned to caller
char	*get_next_line(int fd)
{
	static char	*stash[OPEN_MAX];
	char		*line;

	if (fd < 0 || fd >= OPEN_MAX || BUFFER_SIZE <= 0)
		return (NULL);
	stash[fd] = gnl_read(fd, stash[fd]);
	if (!stash[fd])
		return (NULL);
	line = gnl_get_line(stash[fd]);
	stash[fd] = gnl_new_stash(stash[fd]);
	return (line);
}
/*
#include <fcntl.h>
#include <stdio.h>

// compilation:
// cc [-D BUFFER_SIZE=42] get_next_line.c get_next_line_utils.c
// execution:
// ./a.out [file]
// ./a.out /dev/tty
int	main(int argc, char *argv[])
{
	int		fd;
	char	*line;

	(void)argc;
	fd = open(argv[1], O_RDONLY);
	line = "";
	while (line != NULL)
	{
		line = get_next_line(fd);
		printf("%s", line);
		free(line);
	}
	fd = close(fd);
	return (fd);
}

 */
