/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   my_serv1.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 23:18:35 by anemet            #+#    #+#             */
/*   Updated: 2026/02/05 00:46:29 by anemet           ###   ########.fr       */
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
int max_fd;
int id_count;

fd_set afds, rfds, wfds;

int ids[65536]; // [fd] -> id
char* msgs[65536]; // [fd] -> msg

char buf_read[1001];  // for recv
char buf_write[42]; // for send


int extract_message(char **buf, char **msg)
{
        char    *newbuf;
        int     i;

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
        char    *newbuf;
        int             len;

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
	if (max_fd < 0)
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
	max_fd = (fd > max_fd) ? fd : max_fd;	//update max_fd
	ids[fd] = id_count++;					// get client_id
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

void broadcast_msgs(int fd)
{
	char* msg;

	while (extract_message(&msgs[fd], &msg))
	{
		sprintf(buf_write, "client %d: ", ids[fd]);
		notify_other(fd, buf_write);
		notify_other(fd, msg);
		free(msg);
	}
}

int main(int argc, char *argv[])
{
	// check args
	if (argc < 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	FD_ZERO(&afds);

	int sockfd = create_socket();

	// socket create and verification
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();

	if (listen(sockfd, SOMAXCONN) != 0)
		fatal_error();

	while(1)
	{
		// reset fds
		rfds = wfds = afds;

		// select
		if (select(max_fd + 1, &rfds, &wfds, 0, 0) < 0)
			fatal_error();

		for (int fd = 0; fd <= max_fd; fd++)
		{
			if (!FD_ISSET(fd, &rfds))
				continue;

			if (fd == sockfd)
			{
				// new client
				socklen_t len = sizeof(servaddr);
				int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
				if (client_fd > 0)
				{
					register_client(client_fd);
					break; // new client -> afds changed -> we need new select
				}
			}
			else
			{
				// new data for existing client
				int rec_bytes = recv(fd, buf_read, 1000, 0);
				if (rec_bytes < 0)
				{
					// client disconnected or conn interrupted
					remove_client(fd);
					break; // afds changed, new select
				}
				else
				{
					buf_read[rec_bytes] = '\0';
					msgs[fd] = str_join(msgs[fd], buf_read);
					broadcast_msgs(fd);
				}
			}

		}
	}


}
