/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   my_serv.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 13:06:47 by anemet            #+#    #+#             */
/*   Updated: 2026/02/05 00:46:24 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>


// Globals
int max_fd = 0;		// max fd for select
int id_count = 0;	// id counter for next client

fd_set afds, rfds, wfds;	// active set, read set, write set (man select)

int	ids[65536];		// [fd] -> client id
char* msgs[65536];	// [fd] -> buffers to accumulate messages

char buf_read[1001];	// read buffer for recv
char buf_write[1024];		// write buffer for send


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


void fatal_error()
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

int create_socket()
{
	max_fd = socket(AF_INET, SOCK_STREAM, 0);   // man 7 ip
	if (max_fd < 0)
		fatal_error();
	FD_SET(max_fd, &afds);
	return max_fd;
}

void notify_other(int author, char *msg)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		// send message to writable partner except author
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, msg, strlen(msg), 0);
	}
}

void register_client(int fd)
{
	max_fd = (fd > max_fd) ? fd : max_fd; // update max_fd for select
	ids[fd] = id_count++;				// update id_count
	msgs[fd] = NULL;					// init msgs pointer
	FD_SET(fd, &afds);
	sprintf(buf_write, "server: client %d just arrived\n", fd);
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

	while (extract_message(&msgs[fd], &msg))   // extract while there is new line
	{
		sprintf(buf_write, "client %d: %s", ids[fd], msg);	// format message
		notify_other(fd, buf_write);						// broadcast message
		free(msg);						// free msg malloc'ed by extract_message()
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&afds);  // reset active fd set
	// create socket, add fd to afds, set max_fd
	int sockfd = create_socket();

	// man 7 ip
	// struct sockaddr_in {
	// 	sa_family_t    sin_family; /* address family: AF_INET */
	// 	in_port_t      sin_port;   /* port in network byte order */
	// 	struct in_addr sin_addr;   /* internet address */
	// };

	// /* Internet address. */
	// struct in_addr {
	// 	uint32_t       s_addr;     /* address in network byte order */
	// };

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr)); // reset struct to be on the safe side

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	// htonl: host to network long (4 bytes)
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1 -> 127*256^3 + 1
	servaddr.sin_port = htons(atoi(argv[1]));	// htons: host to network short (2 bytes)

	// Bind socket to given IP and verification (man bind)
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();

	// mark sockfd passive -> will be used to accept new connections with `accept()`
	if (listen(sockfd, SOMAXCONN) != 0)  // SOMAXCONN -> max size of Listen Backlog
		fatal_error();


	while (1)
	{
		// reset fd sets
		rfds = wfds = afds;

		// man select() - wait for fd activity
		// int select(int nfds, *rfds, *wfds, *efds, *timeout);
		// efds = NULL: we don't use exception set
		// timeout = NULL: we're waiting indefinitely until something happens
		if (select(max_fd+1, &rfds, &wfds, 0, 0) < 0)
			fatal_error();

		// scan through all fd-s if there is data to read
		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))  // if no activity -> continue with next fd
				continue;

			if (fd == sockfd)
			{
				// socket activity: new client trying to connect
				// man accept
				// int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
				socklen_t addrlen = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addrlen);
				if (client_fd > 0)
				{
					register_client(client_fd);
					break;	// restart loop, afds changed, fresh select is needed
				}
			}
			else
			{
				// existing client, read data (man recv)
				int read_bytes = recv(fd, buf_read, 1000, 0);

				if (read_bytes <= 0)		// disconnect ?
				{
					remove_client(fd);
					break; // afds changed -> new select
				}
				else
				{
					// existing client, new data received
					buf_read[read_bytes] = '\0'; // NULL terminate new data
					msgs[fd] = str_join(msgs[fd], buf_read); // append new data
					send_msgs(fd); // broadcast completed lines
				}
			}

		}

	}

}

