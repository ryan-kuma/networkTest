#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <event2/event.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

void read_cb(evutil_socket_t fd, short what, void *arg)
{
	char buf[1024] = {0};
	struct event *ev = arg;
	int len = read(fd, buf, sizeof(buf)-1);

	if (len <= 0) {
		if (len == -1)
			perror("read");
		else if (len == 0)
			fprintf(stderr, "connection closed\n");
		event_del(ev);
		event_base_loopbreak(event_get_base(ev));
		return;
	}

	buf[len]='\0';
	fprintf(stdout,"data len = %d, buf = %s\n", len ,buf);
}

static void signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event_base *base = arg;
	event_base_loopbreak(base);
}

int main()
{
	const char* fifo = "fifo";
	unlink(fifo);
	if (mkfifo(fifo, 0600) == -1)
	{
		perror("mkfifo");
		exit(1);
	}

	int fd = open(fifo, O_RDONLY | O_NONBLOCK, 0);

	struct event_base *base = event_base_new();

	struct event *signal_int = evsignal_new(base, SIGINT, signal_cb, base);
	event_add(signal_int ,NULL);

	struct event* ev = event_new(base, fd, EV_READ | EV_PERSIST, read_cb, event_self_cbarg());

	event_add(ev, NULL);

	event_base_dispatch(base);
	event_base_free(base);
	close(fd);
	unlink(fifo);
	libevent_global_shutdown();

	return 0;
}
