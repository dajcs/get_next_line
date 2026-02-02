#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>  // fd_set


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
int		ids[65535];			// Array [FD] -> Client ID
char	*msgs[65535];		// Array [FD] -> Buffered string (accumulated data)
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


int main(int argc, char **argv)
{
	// 1. Argument Check
	if (argc != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	// 2. TODO: Create socket, bind, and listen

	int sockfd, connfd;

	// Create socket (AF_INET -> Address Family: Internet)
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		fatal_error();

//>>>>>>>>>>> Start copy/paste from main

	struct sockaddr_in servaddr;  // man 7 ip
	bzero(&servaddr, sizeof(servaddr)); // reset to 0

	// Assign Family, IP (127.0.0.1) and PORT
	// htonl: host to network long (4 bytes), htons: host to network short (2 bytes)
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1 -> 127*256^3 + 1
	servaddr.sin_port = htons(atoi(argv[1]));

	// Bind socket
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();

	// Listen
	if (listen(sockfd, SOMAXCONN) != 0)   // the main uses 10, SOMAXCONN: 128..4096
		fatal_error();

//<<<<<<<<<<<<<<<<< End copy/paste from main

	// TODO: Implement select() for non-blocking I/O
	//			Accept connections and Handle Clients

	// 3. Initialize Select Logic
	FD_ZERO(&current_sockets);
	FD_SET(sockfd, &current_sockets); // Add server socket FD to the set
	max_fd = sockfd;

	// 4. Main Select Loop
	while (1)
	{
		// select modifies the fd_set, so we must copy it every loop
		read_sockets = current_sockets;

		// Wait for activity. This blocks until and FD is ready
		// We use NULL for timeout to wait indefinitely
		// select(max_fd+1, *read_fds, *write_fds, *except_fds, *timeout)
		if (select(max_fd + 1, &read_sockets, 0, 0, 0) < 0)
			fatal_error();

		// check all file descriptors to see which one is ready
		for (int fd = 0; fd <= max_fd; fd++)
		{
			// if this fd is NOT in the ready set, skip it
			if (!FD_ISSET(fd, &read_sockets))
				continue;

			// CASE A: The Server Socket is ready -> New Connection
			if (fd == sockfd)
			{
				connfd = accept(sockfd, NULL, NULL);
				if (connfd < 0)
					fatal_error();

				// Update max_fd
				if (connfd > max_fd)
					max_fd = connfd;

				// Assign ID and add to set
				ids[connfd] = g_id++;
				msgs[connfd] = NULL; // init buffer
				FD_SET(connfd, &current_sockets);

				// Notify everyone else
				sprintf(buf_write, "server: client %d just arrived\n", ids[connfd]);
				send_to_all(connfd, buf_write);
			}

			// CASE B: Client Socket is ready -> Read Data
			else
			{
				int ret = recv(fd, buf_read, 1000, 0);

				// if Client disconnected (0) or Error (-1)
				if (ret <= 0)
				{
					sprintf(buf_write, "server: client %d just left\n", ids[fd]);
					send_to_all(fd, buf_write);

					// Cleanup
					free(msgs[fd]);
					msgs[fd] = NULL;
				}
				// else: Data received
				else
				{
					buf_read[ret] = '\0'; // Null-terminate received data
					msgs[fd] = str_join(msgs[fd], buf_read); // Add to client's buffer

					// Check if we have complete lines (ending in \n)
					char *line = NULL;
					while (extract_message(&msgs[fd], &line))
					{
						// Format and broadcast the message
						sprintf(buf_write, "client %d: %s", ids[fd], line);
						send_to_all(fd, buf_write);
						free(line); // extract_message mallocs `line`, we must free
					}
				}
			}


		}
	}

	return 0;
}
/*
### How to Test
1.  Compile: `gcc -Wall -Wextra -Werror mini_serv.c -o mini_serv`
2.  Run server: `./mini_serv 8080`
3.  Open 3 different terminals.
4.  Terminal 1: `nc 127.0.0.1 8080` (Client 0)
5.  Terminal 2: `nc 127.0.0.1 8080` (Client 1 - Should see "client 0 arrived")
6.  Type "hello" in Term 1. Term 2 should see "client 0: hello".
*/
