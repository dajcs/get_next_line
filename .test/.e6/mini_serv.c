#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>  // select, fd_set

// Globals
int max_fd = 0;		// track max_fd for select()
int id_count = 0;	// counter for client id

fd_set afds, rfds, wfds;	// active/read/write fd sets

int ids[65536];		// [fd] -> client id
char *msgs[65536];	// [fd] -> accumulate msgs buffer

char buf_write[1024];	// buffer for send
char buf_read[1001];	// buffer for recv


// >>>>>>>> PROVIDED FUNCTION <<<<<<<<<<<<

// check for a complete line in the msgs buffer (**buf)
// if complete line: extract line to (**msg) and return 1
// 	otherwise return 0
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


// >>>>>>>> PROVIDED FUNCTION <<<<<<<<<<<<

// append new string from recv (*add) to the msgs buffer (*buf)
// return the joined string
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

void fatal_error()
{
	write(2, "fatal error\n", 12);
	exit(1);
}

// Create the server's listening socket (TCP/IPv4)
// Returns the socket fd, which is also stored in max_fd
int create_socket()
{
	max_fd = socket(AF_INET, SOCK_STREAM, 0);	// TCP socket
	if (max_fd == -1)
		fatal_error();
	FD_SET(max_fd, &afds);	// Add server socket to active set
	return max_fd;
}

// send msg to all in writable set, except the author
void notify_other(int author, char *msg)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		// check if fd is in the writable set and is not the author
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, msg, strlen(msg), 0);
	}
}

// Called after a new client connects via accept()
// Assigns a unique ID, initializes message buffer and notifies others
void register_client(int fd)
{
	max_fd = (fd > max_fd) ? fd : max_fd;	// update max_fd if needed
	ids[fd] = id_count++;					// assign next available client ID
	msgs[fd] = NULL;						// Init empty message buffer
	FD_SET(fd, &afds);						// add fd to the active set
	sprintf(buf_write, "server: client %d just arrived\n", ids[fd]); // compose notify msg
	notify_other(fd, buf_write);			// Announce arrival to other clients
}

// Called when a client disconnects (recv returns 0 or error -1)
// Notifies others, removes from active set, and cleans up resources
void remove_client(int fd)
{
	sprintf(buf_write, "server: client %d just left\n", ids[fd]); // compose notify msg
	notify_other(fd, buf_write);			// notify all other
	FD_CLR(fd, &afds);						// remove from active set
	free(msgs[fd]);							// free msgs buffer
	close(fd);								// close fd
}

// Process and broadcast all complete messages from a client's buffer
// A complete message ends with '\n'. Multiple messages may be queued
void send_msgs(int fd)
{
	char *msg;

	// Extract and send each complete message (loop handles multiple lines one-by-one)
	while(extract_message(&msgs[fd], &msg))
	{
		sprintf(buf_write, "client %d: %s", ids[fd], msg);	// compose message
		notify_other(fd, buf_write);						// broadcast the message
		free(msg);											// free the consumed msg buffer
	}
}

int main(int argc, char* argv[]) {

	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&afds);  // 1st step: clear all fd-s from the active set
	int sockfd = create_socket();

	// create server socket (man 7 ip)
	struct sockaddr_in servaddr;
	socklen_t addrlen = sizeof(servaddr);	// 16

	bzero(&servaddr, addrlen);	// clear servaddr (to be on safe side)

	// Assign Family, IP (127.0.0.1) and PORT
	// htonl: host to network long (4 bytes), htons: host to network short (2 bytes)
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1 -> 127*256^3 + 1
	servaddr.sin_port = htons(atoi(argv[1]));

	// Bind server socket to given IP
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();
	// Listen: Mark socket as passive (accepting connections)
	if (listen(sockfd, SOMAXCONN))   // the given main uses 10, we use SOMAXCONN: 128..4096
		fatal_error();

	// Main event loop - runs forever, processing client events
	while (1)
	{
		// Reset read/write sets from active set before each select()
		rfds = wfds = afds;

		// Block until at least one fd has activity (readable or writable)
		// except fds: NULL, timeout: NULL -> wait indefinitely
		if (select(max_fd+1, &rfds, &wfds, 0, 0) < 0)
			fatal_error();

		// scan through all fd-s to find the ones with incoming event
		for (int fd = 0; fd <= max_fd; fd++)
		{
			// if no activity on fd -> continue
			if (!FD_ISSET(fd, &rfds))
				continue;

			if (fd == sockfd)
			{
				// event on sockfd -> new client is trying to connect
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addrlen);

				if (client_fd > 0)
				{
					register_client(client_fd);
					break;  // afds changed, new select needed
				}
			}
			else
			{
				// read event on existing client
				int read_bytes = recv(fd, buf_read, 1000, 0);

				// read_bytes < 0: connection broken
				// read_bytes == 0: clean disconnect
				if (read_bytes <= 0)
				{
					remove_client(fd);
					break;		// afds changed, new select needed
				}
				else
				{
				// Accumulate received data and process any complete messages
					buf_read[read_bytes] = '\0';				// null terminate recv chunk
					msgs[fd] = str_join(msgs[fd], buf_read);	// append chunk to msgs buffer
					send_msgs(fd);								// broadcast msgs to other clients
				}
			}
		}

	}

}

/*
	Test:
		- start server:
			./mini_serv 8080
		- connect clients from other terminals:
			nc 127.0.0.1 8080
		- enter message in client terminals
		- Ctrl-C on client terminals and server terminal
	Note:
		- after Ctrl-C sockets might remain in a hanging state,
			close all terminals in order to execute a new experiment
*/
