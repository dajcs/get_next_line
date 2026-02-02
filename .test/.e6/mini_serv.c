#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip.h>
#include <sys/select.h>	// fd_set


// Global state for the chat server
int		count = 0;		// count = next client ID
int		max_fd = 0;		// max_fd = highest fd for select()
int		ids[65536];		// maps fd -> client ID (for display purposes)
char	*msgs[65536];	// maps fd -> incomplete message buffer (accumulates until '\n')

fd_set	rfds;	// rfds = read fd set (who has data to read)
fd_set	wfds;	// wfds = write fd set (who can receive data)
fd_set	afds;	// afds = active fd set (all connected clients + server socket)

char	buf_read[1001];		// buffer for recv
char	buf_write[42];		// buffer to send client ID join/leave/prefix


// START COPY-PASTE FROM GIVEN MAIN

// Extract a complete message (ending with '\n') from the buffer
// - if a newline is found, *msg is set to the message(including '\n')
//   *buf is updated to contain the remaining data after the extracted message
// - Return 1 if a line was found, 0 if not, -1 on allocation error
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
			// Allocate new buffer for remaining data after the newline
			// *buf + i -> pointing to '\n'; +1 next char after '\n'
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);	// copy remainder to newbuf
			*msg = *buf;					// message is the original buffer
			(*msg)[i + 1] = 0;				// terminate message after newline
			*buf = newbuf;					// update buf to point to remainder
			return (1);
		}
		i++;
	}
	return (0);		// No complete message yet (no newline found)
}

// Concatenates the new data (`add`) to the existing buffer (`buf`)
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
	free(buf);				// free old buffer (ownership transferred)
	strcat(newbuf, add);
	return (newbuf);
}


// END COPY-PASTE




// Write "Fatal error" and exit
void fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}


/*
	notify_other()
	- Broadcast a message to all connected clients EXCEPT the author
	- Uses wfds to check which clients are ready to receive data
*/
void	notify_other(int author, char *str)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		// send only to writable fds and not the message author
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, str, strlen(str), 0);
	}
}


/*
	register_client()
	- Called when a new client connects via accept()
	- Assigns a unique ID, initializes message buffer and notifies others
*/
void	register_client(int fd)
{
	max_fd = fd > max_fd ? fd : max_fd;	// update max_fd for select()
	ids[fd] = count++;					// assign next available client ID
	msgs[fd] = NULL;					// Init empty message buffer
	FD_SET(fd, &afds);					// Add to active fd set
	sprintf(buf_write, "server: client %d just arrived\n", ids[fd]);
	notify_other(fd, buf_write);		// Announce arrival to other clients
}


/*
	remove_client()
	- Called when a client disconnects (recv returns 0 or error -1)
	- Notifies others, cleans up resources, and removes from active set
*/
void	remove_client(int fd)
{
	sprintf(buf_write, "server: client %d just left\n", ids[fd]);
	notify_other(fd, buf_write);	// Announce departure to other clients
	FD_CLR(fd, &afds);				// Remove from active fd set
	free(msgs[fd]);					// Free accumulated message buffer
	close(fd);						// close client socket fd
}


/*
	send_msg()
	- Process and broadcast all complete messages from a client's buffer
	- A complete message ends with '\n'. Multiple messages may be queued
*/
void	send_msg(int fd)
{
	char *msg;

	// Extract and send each complete message (loop handles multiple '\n')
	while (extract_message(&(msgs[fd]), &msg))
	{
		sprintf(buf_write, "client %d: ", ids[fd]);	// Format sender prefix
		notify_other(fd, buf_write);				// Send prefix to others
		notify_other(fd, msg);						// Send actual message
		free(msg);									// Free sent message
	}
}


/*
	create_socket()
	- Create the server's listening socket (TCP/IPv4)
	- Returns the socket fd, which is also stored in max_fd
*/
int	create_socket()
{
	max_fd = socket(AF_INET, SOCK_STREAM, 0);	// TCP socket
	if (max_fd < 0)
		fatal_error();
	FD_SET(max_fd, &afds);	// Add server socket to active set
	return max_fd;
}


int main(int argc, char **argv)
{
	// 1. Argument Check
	if (argc != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&afds);
	int sockfd = create_socket();

//>>>>>>>>>>> START COPY-PASTE FROM MAIN

	struct sockaddr_in servaddr;  // man 7 ip
	bzero(&servaddr, sizeof(servaddr)); // reset to 0

	// Assign Family, IP (127.0.0.1) and PORT
	// htonl: host to network long (4 bytes), htons: host to network short (2 bytes)
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1 -> 127*256^3 + 1
	servaddr.sin_port = htons(atoi(argv[1]));

	// Bind socket
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))))
		fatal_error();

	// Listen: Mark socket as passive (accepting connections)
	if (listen(sockfd, SOMAXCONN))   // the main uses 10, SOMAXCONN: 128..4096
		fatal_error();

//<<<<<<<<<<<<<<<<< END COPY-PASTE from main


	// 4. Main event loop - runs forever, processing client events
	while (1)
	{
		// Reset read/write sets from active set before each select()
		rfds = wfds = afds;

		// Block until at least one fd has activity (readable or writable)
		// except fds: NULL, timeout: NULL
		if (select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
			fatal_error();

		// Check each fd to see if it has data to read
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))	// Skip if no data to read
				continue;

			if (fd == sockfd)
			{
				// Event on sockfd -> new client trying to connect
				socklen_t addr_len = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len);
				if (client_fd >= 0)
				{
					register_client(client_fd);
					break;	// Restart loop (afds changed, need fresh select)
				}
			}
			else
			{
				// Client socket is readable = client sent data or disconnected
				int read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes <= 0)
				{
					// 0 = clean disconnect, <0 = error -> remove client
					remove_client(fd);
					break; // Restart loop (afds changed, need fresh select)
				}
				// Accumulate received data and process any complete messages
				buf_read[read_bytes] = '\0';
				msgs[fd] = str_join(msgs[fd], buf_read);
				send_msg(fd);	// Broadcast any complete messages
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
