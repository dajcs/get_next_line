#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>


/*  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	Start of given code in main.c
*/

// Extract a single line (ending in \n) from the buffer
// Return 1 if a line was found, 0 if not, -1 on allocation error
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			// storing the remaining string in newbuf
			// *buf + i -> pointing to '\n'; +1 next char after '\n'
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

// Concatenates the new data (add) to the existing buffer (buf)
// free orig `buf`, return `newbuf`
char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

/*
	End of given code in main.c
	<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/


// -------------------------
// Server Implementation
// -------------------------

// Global variables for managing state
int		max_fd = 0;			// The highest file descriptor currently open
int		ids[65535];			// Array mapping [FD] -> Client ID
char	*msgs[65535];		// Array mapping [FD] -> Buffered string (accumulated data)
fd_set	current_sockets;	// Master set of all open sockets
fd_set	read_sockets;		// Temp set passed to select()
char	buf_read[4096];		// Buffer for raw input from recv
char	buf_write[4096];	// Buffer for formatted output messages
int		g_id = 0;			// Counter for assigning Client IDs


// Write "Fatal error" and exit
void fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

/*
	send_to_all() - send message to everyone EXCEPT the sender (`except_fd`)

	if `except_fd` is the server socket or invalid,
	we can pass e.g. `-1` if we want to send to absolutely everyone
*/
void send_to_all(int except_fd, char *str)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		// Check if fd is an active socket and isn't the sender
		if (FD_ISSET(fd, &current_sockets) && fd != except_fd)
		{
			write(fd, str, strlen(str));
		}
	}
}


int main(int ac, char **av)
{
	// 1. Argument Check
	if (ac != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	// Socket setup
	int sockfd;
	struct sockaddr_in servaddr;

	// TODO: Create socket, bind, and listen

	// TODO: Implement select() for non-blocking I/O

	// TODO: Accept connections and handle clients

	return 0;
}
