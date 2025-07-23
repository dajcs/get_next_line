#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define OPEN_MAX 1024
#ifndef BUFFER_SIZE
# define BUFFER_SIZE 42
#endif
int gnl_strlen(char *s)
{
	int i;

	i = 0;
	while(s && s[i])
		i++;
	return i;
}

// return ptr to first occurence of 'c' ('c' can be '\0' as well)
// return NULL if 'c' not in 's'
char *gnl_strchr(char *s, int c)
{
	int i;

	i = 0;
	while (s && s[i])
	{
		if (s[i] == (char)c)
			return s + i;
		i++;
	}
	if (s && c == 0)
		return (s + i);
	return NULL;
}

// malloc str
// return str = s1 + s2
// free s1
char *gnl_strjoin(char *s1, char *s2)
{
	int i;
	int j;
	int len;
	char *str;

	i = 0;
	j = 0;
	len = gnl_strlen(s1) + gnl_strlen(s2);
	str = (char *)malloc(sizeof(char) * (len + 1));
	if (!str)
		return NULL;
	while(s1 && s1[i])
	{
		str[i] = s1[i];
		i++;
	}
	j = 0;
	while(s2 && s2[j])
	{
		str[i + j] = s2[j];
		j++;
	}
	str[len] = '\0';
	free(s1);
	return str;
}

// allocate, copy and return a slice of s[start.. start + len]
// [s......s+start...s+start+len...]
char *gnl_substr(char *s, int start, size_t len)
{
	size_t i;
	char *sub;

	if (!s || start >= gnl_strlen(s))
		return NULL;
	if (len > (size_t)gnl_strlen(s+start))
		len = gnl_strlen(s+start);
	sub = (char *)malloc(len + 1);
	if (!sub)
		return NULL;
	i = 0;
	while(i < len && s[start + i])
	{
		sub[i] = s[start + i];
		i++;
	}
	sub[i] = '\0';
	return sub;
}
char *gnl_free(char *buf)
{
	free(buf);
	return NULL;
}


//returns a copy of the first complete line (including '\n' if present)
char *gnl_get_line(char *stash)
{
	size_t i;
	char *line;

	if (!stash || !stash[0])
		return NULL;
	i = 0;
	while (stash[i] && stash[i] != '\n')
		i++;
	if (stash[i] == '\n')
	i++;
	line = gnl_substr(stash, 0, i);
	return line;
}


// fill stash until '\n' in stash or EOF
// read() returns nr of bytes read
// new stash = old stash + buf // gnl_strjoin frees old stash
char *gnl_read(int fd, char *stash)
{
	char *buf;
	int rd;

	buf = malloc(BUFFER_SIZE + 1);
	if (!buf)
		return NULL;
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
char *gnl_new_stash(char *stash)
{
	size_t i;
	char *new;

	i = 0;
	while (stash[i] && stash[i] != '\n')
		i++;
	if (!stash[i])
	{
		free(stash);
		return 0;
	}
	new = gnl_substr(stash, i+1, gnl_strlen(stash + i + 1));
	free(stash);
	return new;
}

// stash[fd] keeps whatever was read from file, but not yet returned to the caller
char *get_next_line(int fd)
{
	static char *stash[OPEN_MAX];
	char *line;

	if (fd < 0 || fd >= OPEN_MAX || BUFFER_SIZE <= 0)
		return NULL;
	stash[fd] = gnl_read(fd, stash[fd]);
	if (!stash[fd])
		return NULL;
	line = gnl_get_line(stash[fd]);
	stash[fd] = gnl_new_stash(stash[fd]);
	return line;
}

/*
int main(int argc, char *argv[])
{
	// char *s1;
	// char *s2;
	// char c;
	// char *res;
	int fd;
	char *line;

	if (argc < 2)
	{
		printf("gempa\n");
		return 1;
	}
	// if (atoi(argv[1]) == 0)
	// 	s = NULL;
	// else
	// 	s = argv[1];
	// c = argv[2][0];
	// res = gnl_strchr(s, c);
	// printf("res: %s\n", res);
	// s1 = argv[1];
	// s2 = argv[2];
	// res = gnl_strjoin(s1, s2);
	// printf("res: \"%s\"\n", res);
	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
		perror(argv[1]);
	line = get_next_line(fd);
	while (line)
	{
		printf("fd = %d,\"%s\"\n", fd, line);
		line = get_next_line(fd);
	}
	printf("bye\n");
}
 */
