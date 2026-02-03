#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>

// Global variables
int idcount = 0;			// track id
int max_fd = 0;				// highest fd for select
fd_set afds, rfds, wfds;	// active/read/write fd set
int ids[65536];				// [fd] -> id
char *msgs[65536];			// [fd] -> incomplete message buffer (accumulates till '\n')
char buf_read[1001];		// buffer for recv
char buf_write[42];			// buffer to send client ID join/leave/prefix

// These functions are provided in the exam
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

// Error handling function
void fatal_error() {
	write(2, "Fatal error\n", 12);
	exit(1);
}


int create_socket()
{
	max_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (max_fd < 0)
		fatal_error();
	FD_SET(max_fd, &afds);
	return max_fd;
}

void notify_other(int author, char* msg)
{
	// send message to writable partners
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, msg, strlen(msg), 0);
	}
}

void register_client(int fd)
{
	max_fd = fd > max_fd ? fd : max_fd;  // update max_fd for select
	ids[fd] = idcount++;
	msgs[fd] = NULL;
	FD_SET(fd, &afds);
	sprintf(buf_write, "server: client %d just arrived\n", ids[fd]);
	notify_other(fd, buf_write);
}

void remove_client(int fd)
{
	sprintf(buf_write, "server: client %d just left\n", ids[fd]);
	notify_other(fd, buf_write);
	FD_CLR(fd, &afds);
	free(msgs[fd]);
	close(fd);
}

void send_msgs(int fd)
{
	char *msg;
	sprintf(buf_write, "client %d: ", ids[fd]);
	notify_other(fd, buf_write);

	while (extract_message(&(msgs[fd]), &msg))
	{
		notify_other(fd, msg);
		free(msg);
	}
}

int main(int argc, char **argv) {
	// Check arguments
	if (argc != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	// Socket setup
	FD_ZERO(&afds);
	int sockfd = create_socket();

	// Create server, bind, listen
	struct sockaddr_in servaddr;  // man 7 ip
	bzero(&servaddr, sizeof(servaddr));

	// assign family AF_INET, ip, port
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1 -> 127*256^3 + 1
	servaddr.sin_port = htons(atoi(argv[1]));

	// bind
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))))
		fatal_error();

	// listen
	if (listen(sockfd, SOMAXCONN)) // 10 -> SOMAXCONN
		fatal_error();


	// Implement select() for non-blocking I/O
	while(1)
	{
		// reset sets
		rfds = wfds = afds;

		// select - waits for fd activity
		if (select(max_fd + 1, &rfds, &wfds, 0, 0) < 0)
			fatal_error();

		// check each fd if has data to read
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))  // skip if no event
				continue;

			if (fd == sockfd)
			{
				// Event on sockfd -> new client trying to connect
				socklen_t addr_len = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len);
				if (client_fd >= 0)
				{
					register_client(client_fd);
					break; // restart loop, afds changed, we need fresh select
				}
			}
			else
			{
				// read client
				int read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes <= 0)  // disconnect
				{
					remove_client(fd);
					break; // afds changed -> new select
				}
				// terminate received data with '\0' and append
				buf_read[read_bytes] = '\0';
				msgs[fd] = str_join(msgs[fd], buf_read);
				send_msgs(fd);  // broadcast completed lines
			}
		}
	}

	return 0;
}
