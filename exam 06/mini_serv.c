#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int g_id;
int sockfd;

typedef struct s_client
{
	int id;
	int fd;
	struct s_client *next;
}				t_client;


void	error(char *str)
{
	write(2, str, strlen(str));
	if (sockfd != -1)
		close(sockfd);
	exit(1);
}

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

void	add_client(t_client **lst, int fd)
{
	t_client *new = 0;

	if (!(new = malloc(sizeof(t_client))))
		error("Fatal error\n");
	new->id = g_id;
	new->fd = fd;
	new->next = *lst;
	*lst = new;
}

void	rm_client(t_client **lst, int fd)
{
	t_client *tmp = *lst;
	t_client *prev = 0;

	if (tmp != 0 && tmp->fd == fd)
	{
		*lst = tmp->next;
		close(tmp->fd);
		free(tmp);
	}
	else
	{
		while (tmp != 0 && tmp->fd != fd)
		{
			prev = tmp;
			tmp = tmp->next;
		}
		if (tmp != 0)
		{
			prev->next = tmp->next;
			close(tmp->fd);
			free(tmp);
		}
	}
}

void	send_all(t_client *lst, char *str, int fd)
{
	for (; lst; lst = lst->next)
		if (lst->fd != fd)
			send(lst->fd, str, strlen(str), 0);
}

void	init_set(t_client *lst, fd_set *readfds)
{
	FD_ZERO(readfds);
	for (; lst; lst = lst->next)
		FD_SET(lst->fd, readfds);
	FD_SET(sockfd, readfds);
}

int		main(int ac, char **av)
{
	g_id = 0;
	sockfd = -1;
	int port = 0;
	int connfd = 0;
	struct sockaddr_in servaddr, cli;
	socklen_t len = sizeof(cli);

	fd_set writefds;

	t_client *client = 0;
	t_client *tmp = 0;

	char buf[4200];
	char *msg = 0;
	char *extr = 0;

	if (ac != 2)
		error("Wrong number of arguments\n");

	port = atoi(av[1]);
	if (port <= 0)
		error("Fatal error\n");

	// socket create and verification
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		error("Fatal error\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		error("Fatal error\n");
	if (listen(sockfd, 10) != 0)
		error("Fatal error\n");

	while (1)
	{
		init_set(client, &writefds);
		select(FD_SETSIZE, &writefds, 0, 0, 0);

		if (FD_ISSET(sockfd, &writefds))
		{
			if ((connfd = accept(sockfd, (struct sockaddr *)&cli, &len)) < 0)
				error("Fatal error\n");
			//fcntl(connfd, F_SETFL, O_NONBLOCK); <-- testing
			bzero(buf, sizeof(buf));
			add_client(&client, connfd);
			sprintf(buf, "server: client %d just arrived\n", g_id++);
			send_all(client, buf, connfd);
		}

		tmp = client;
		while (tmp)
		{
			int id = tmp->id;
			connfd = tmp->fd;
			tmp = tmp->next;

			if (FD_ISSET(connfd, &writefds))
			{
				int i = 0;
				int size = 0;

				bzero(buf, sizeof(buf));
				while ((size = recv(connfd, buf, 4097, 0)) > 0)
				{
					buf[size] = 0;
					i += size;
					msg = str_join(msg, buf);
				}

				if (i == 0)
				{
					bzero(buf, sizeof(buf));
					sprintf(buf, "server: client %d just left\n", id);
					send_all(client, buf, connfd);
					rm_client(&client, connfd);
				}
				else if (i > 0)
				{
					char *tmp_str = 0;

					while ( extract_message(&msg, &extr) )
					{
						if (!(tmp_str = malloc(40 + strlen(extr))))
							error("Fatal error\n");
						sprintf(tmp_str, "client %d: %s", id, extr);
						send_all(client, tmp_str, connfd);
						free(extr);
						free(tmp_str);
						extr = 0;
						tmp_str = 0;
					}
				}
				free(msg);
				msg = 0;
			}
		}
	}
}

