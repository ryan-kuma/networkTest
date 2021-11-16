#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/queue.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#define ERR_EXIT(m) \
	do { \
		perror(m); \
		exit(EXIT_FAILURE); \
	} while(0); 

#define SERVER_PORT 5555

struct client{
	int fd;
	struct bufferevent *buf_ev;

	TAILQ_ENTRY(client) entries;
};

static struct event_base *evbase;

TAILQ_HEAD(, client) client_tailq_head;

int setnonblock(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFL);
	if (flags < 0);
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;
	
	return 0;
}

void buffered_on_read(struct bufferevent *bevent, void *arg)
{
	struct client *thiscli = (struct client*) arg;
	struct client *cli;

	uint8_t data[8192];
	size_t n;

	for(;;) {
		n = bufferevent_read(bevent, data, sizeof(data));
		if (n <= 0){
			break;
		}
		bufferevent_write(bevent, data, n);
	}
}

void buffered_on_error(struct bufferevent *bevent, short what, void *arg)
{
	struct client *cli = (struct client *) arg;
	if (what & BEV_EVENT_EOF) {
		printf("client disconnected.\n");
	}
	else {
		ERR_EXIT("client socket error, disconnected.\n");
	}

	TAILQ_REMOVE(&client_tailq_head, cli, entries);
	bufferevent_free(cli->buf_ev);
	close(cli->fd);
	free(cli);
}

void on_accept(int fd, short ev, void *arg)
{
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	struct client *client;
	client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
	if(client_fd < 0) {
		ERR_EXIT("accept failed");
		return;
	}

	if (setnonblock(client_fd) < 0)
		 ERR_EXIT("failed to set client socket non-blocking");

	client = (struct client *)calloc(1, sizeof(*client));
	if (client == NULL)
		 ERR_EXIT("malloc failed");
	
	client->fd = client_fd;

	client->buf_ev = bufferevent_socket_new(evbase, client_fd, 0);
	bufferevent_setcb(client->buf_ev, buffered_on_read, NULL, buffered_on_error, client);

	bufferevent_enable(client->buf_ev, EV_READ);
	TAILQ_INSERT_TAIL(&client_tailq_head, client, entries);
	printf("Accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));

	
}


int main()
{
	int listenfd;
	struct sockaddr_in listen_addr;
	struct event ev_accept;

	evbase = event_base_new();
	TAILQ_INIT(&client_tailq_head);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0)
		ERR_EXIT("listen failed");

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(SERVER_PORT);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0)
		 ERR_EXIT("bind failed");

	if (listen(listenfd, 5) < 0)
		ERR_EXIT("listen failed");

	int reuseaddr_on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on));

	if (setnonblock(listenfd) < 0)
		ERR_EXIT("nonblocking error");

	event_assign(&ev_accept, evbase, listenfd, EV_READ|EV_PERSIST, on_accept, NULL);
	event_add(&ev_accept, NULL);
	event_base_dispatch(evbase);

	return 0;
}
