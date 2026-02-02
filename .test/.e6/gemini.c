#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

// ----------------------------------------------------------------------
// PROVIDED HELPER FUNCTIONS (FROM TEMPLATE)
// ----------------------------------------------------------------------

// Extracts a single line (ending in \n) from the buffer.
// Returns 1 if a line was found, 0 if not, -1 on allocation error.
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

// Concatenates the new data (add) to the existing buffer (buf).
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

// ----------------------------------------------------------------------
// SERVER IMPLEMENTATION
// ----------------------------------------------------------------------

// Global variables for managing state
int     max_fd = 0;             // The highest file descriptor currently open
int     ids[65535];             // Array mapping FD -> Client ID
char    *msgs[65535];           // Array mapping FD -> Buffered string (accumulated data)
fd_set  current_sockets;        // Master set of all open sockets
fd_set  read_sockets;           // Temp set passed to select()
char    buf_read[4096];         // Buffer for raw input from recv
char    buf_write[4096];        // Buffer for formatted output messages
int     g_id = 0;               // Counter for assigning Client IDs

// Writes "Fatal error" and exits
void fatal_error() {
	write(2, "Fatal error\n", 12);
	exit(1);
}

// Writes "Wrong number of arguments" and exits
void args_error() {
	write(2, "Wrong number of arguments\n", 26);
	exit(1);
}

// Sends a message to everyone EXCEPT the sender (except_fd)
// If except_fd is the server socket or invalid, you can pass a value like -1
// if you want to send to absolutely everyone.
void send_to_all(int except_fd, char *str) {
	for (int fd = 0; fd <= max_fd; fd++) {
		// Check if the fd is part of our active sockets and isn't the sender
		if (FD_ISSET(fd, &current_sockets) && fd != except_fd) {
			write(fd, str, strlen(str));
		}
	}
}

int main(int ac, char **av)
{
	// 1. Argument Check
	if (ac != 2)
		args_error();

	// 2. Setup Server Socket
	int sockfd, connfd;
	struct sockaddr_in servaddr;

	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		fatal_error();

	bzero(&servaddr, sizeof(servaddr));

	// Assign IP (127.0.0.1) and PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Bind socket
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();

	// Listen
	if (listen(sockfd, 10) != 0)
		fatal_error();

	// 3. Initialize Select Logic
	FD_ZERO(&current_sockets);
	FD_SET(sockfd, &current_sockets); // Add server socket to the set
	max_fd = sockfd;

	// 4. Main Loop
	while (1) {
		// select modifies the fd_set, so we must copy it every loop
		read_sockets = current_sockets;

		// Wait for activity. This blocks until an FD is ready.
		// We use NULL for timeout to wait indefinitely.
		if (select(max_fd + 1, &read_sockets, 0, 0, 0) < 0)
			fatal_error();

		// Check all file descriptors to see which one is ready
		for (int fd = 0; fd <= max_fd; fd++) {

			// If this fd is NOT in the ready set, skip it
			if (!FD_ISSET(fd, &read_sockets))
				continue;

			// CASE A: The Server Socket is ready -> New Connection
			if (fd == sockfd) {
				connfd = accept(sockfd, NULL, NULL);
				if (connfd < 0)
					fatal_error();

				// Update max_fd if necessary
				if (connfd > max_fd)
					max_fd = connfd;

				// Assign ID and add to set
				ids[connfd] = g_id++;
				msgs[connfd] = NULL; // Initialize buffer
				FD_SET(connfd, &current_sockets);

				// Notify everyone else
				sprintf(buf_write, "server: client %d just arrived\n", ids[connfd]);
				send_to_all(connfd, buf_write);
			}
			// CASE B: A Client Socket is ready -> Read Data
			else {
				int ret = recv(fd, buf_read, 1000, 0);

				if (ret <= 0) {
					// Client disconnected (0) or Error (-1)
					sprintf(buf_write, "server: client %d just left\n", ids[fd]);
					send_to_all(fd, buf_write);

					// Cleanup
					free(msgs[fd]);
					msgs[fd] = NULL;
					FD_CLR(fd, &current_sockets);
					close(fd);
				}
				else {
					// Data received
					buf_read[ret] = '\0'; // Null-terminate received data
					msgs[fd] = str_join(msgs[fd], buf_read); // Add to client's buffer

					// Check if we have complete lines (ending in \n)
					char *line = NULL;
					while (extract_message(&msgs[fd], &line)) {
						// Format and broadcast the message
						sprintf(buf_write, "client %d: %s", ids[fd], line);
						send_to_all(fd, buf_write);
						free(line); // extract_message mallocs 'line', we must free
					}
				}
			}
		}
	}
	return (0);
}
/*
### Explanation of Concepts

1.  **Architecture (FD Arrays)**:
    Instead of using a `struct client` linked list, we use global arrays `ids[65535]` and `msgs[65535]`.
    *   File Descriptors (FDs) are just integers.
    *   If client with ID 5 is on FD 8, `ids[8]` stores `5`.
    *   This provides instant access (O(1)) without traversing a list.

2.  **`select()`**:
    *   `select` is the core of non-blocking I/O here. It allows the program to sleep until *something* happens (new connection or incoming data).
    *   We maintain `current_sockets` (all open connections) and copy it to `read_sockets` before every call because `select` modifies the set to tell us exactly *which* sockets are ready.

3.  **`extract_message` & `str_join`**:
    *   TCP streams split packets arbitrarily. If a client sends "Hello\n", we might receive "Hel" in one loop and "lo\n" in the next.
    *   We assume `str_join` accumulates data into `msgs[fd]`.
    *   We assume `extract_message` checks `msgs[fd]` for a `\n`. If found, it cuts that line out, returns it, and keeps the rest of the data in the buffer for the next loop.

4.  **Buffer Management**:
    *   We must free `msgs[fd]` when a client disconnects to avoid leaks.
    *   We must free `line` returned by `extract_message` after sending it.

5.  **Addressing 127.0.0.1**:
    *   `2130706433` is the integer representation of `127.0.0.1` -> 127*256^3 + 1
    *   `htonl` converts it to Network Byte Order (Big Endian) required by the struct.

### How to Test
1.  Compile: `gcc -Wall -Wextra -Werror mini_serv.c -o mini_serv`
2.  Run server: `./mini_serv 8080`
3.  Open 3 different terminals.
4.  Terminal 1: `nc 127.0.0.1 8080` (Client 0)
5.  Terminal 2: `nc 127.0.0.1 8080` (Client 1 - Should see "client 0 arrived")
6.  Type "hello" in Term 1. Term 2 should see "client 0: hello".
*/
