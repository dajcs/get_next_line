/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   my_serv.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <student.42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026-02-12 09:45:35 by marvin            #+#    #+#             */
/*   Updated: 2026-02-12 09:45:35 by marvin           ###   ########.fr       */
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

// Global variables
int max_fd;
int id_count;

fd_set afds, rfds, wfds;

int ids[65536];
char *msgs[65536];

char buf_write[1024];
char buf_read[1001];


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
	max_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (max_fd == -1)
		fatal_error();
	FD_SET(max_fd, &afds);
	return max_fd;
}

void notify_other(int author, char *msg)
{
	for (int fd = 0; fd <= max_fd; fd++)
	{
		if (FD_ISSET(fd, &wfds) && fd != author)
			send(fd, msg, strlen(msg), 0);
	}
}

void register_client(int fd)
{
	max_fd = (fd > max_fd) ? fd : max_fd;  // upd max_fd
	ids[fd] = id_count++;
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

	while (extract_message(&msgs[fd], &msg))
	{
		sprintf(buf_write, "client %d: %s", ids[fd], msg);
		notify_other(fd, buf_write);
		free(msg);
	}
}

int main(int argc, char *argv[]) {

	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	int sockfd = create_socket();

	struct sockaddr_in servaddr;
	socklen_t addrlen = sizeof(servaddr);

	// socket create and verification
	bzero(&servaddr, addrlen);

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, addrlen)) != 0)
		fatal_error();
	if (listen(sockfd, SOMAXCONN) != 0)
		fatal_error();

	while(1)
	{
		// reset fd sets to afds
		rfds = wfds = afds;

		if (select(max_fd + 1, &rfds, &wfds, 0, 0) < 0)
			fatal_error();

		// scan all fd-s
		for (int fd = 0; fd <= max_fd; fd++)
		{
			// skip fds without read events
			if (!FD_ISSET(fd, &rfds))
				continue;

			if (fd == sockfd)
			{
				// connection req from new client
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addrlen);
				if (client_fd > 0)
				{
					register_client(client_fd);
					break;  // afds changed, new select needed
				}
			}
			else
			{
				// read event from existing client
				int read_bytes = recv(fd, buf_read, 1000, 0);
				if (read_bytes <= 0)
				{
					// client disconnect
					remove_client(fd);
					break; // afds changed, new select needed
				}
				else
				{
					buf_read[read_bytes] = '\0';
					msgs[fd] = str_join(msgs[fd], buf_read);
					send_msgs(fd);
				}
			}
		}
	}
}

