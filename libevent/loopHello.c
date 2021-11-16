/*
 * XXX This sample code was once meant to show how to use the basic Libevent
 * interfaces, but it never worked on non-Unix platforms, and some of the
 * interfaces have changed since it was first written.  It should probably
 * be removed or replaced with something better.
 *
 * Compile with:
 * cc -I/usr/local/include -o time-test time-test.c -L/usr/local/lib -levent
 */

#include <sys/types.h>

#include <event2/event-config.h>

#include <sys/stat.h>
#ifndef _WIN32
#include <sys/queue.h>
#include <unistd.h>
#endif
#include <time.h>
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

static const char MESSAGE[] = "hello world\n";
static const int PORT = 9995;
static void listener_cb(struct evconnlistener *, evutil_socket_t, 
	struct sockaddr*, int socklen, void *);
static void signal_cb(evutil_socket_t, short, void *);
static void write_eventcb(struct bufferevent *, void *);


struct timeval lasttime;
struct eventp{
	struct event_base *base;
	struct event *timeout;	
};

struct eventp multievent;

static void
timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct timeval newtime, difference;
	struct eventp *pevent = (struct eventp *)arg;
	struct event_base *base = pevent->base;
	struct event *timeout = pevent->timeout;
	double elapsed;

	evutil_gettimeofday(&newtime, NULL);
	evutil_timersub(&newtime, &lasttime, &difference);
	elapsed = difference.tv_sec +
	    (difference.tv_usec / 1.0e6);

	printf("timeout_cb called at %d: %.3f seconds elapsed.\n",
	    (int)newtime.tv_sec, elapsed);

	
	struct bufferevent *bev;
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if(!bev){
		fprintf(stderr, "error constructing bufferevent\n");	
		event_base_loopbreak(base);
		return ;
	}
	bufferevent_setcb(bev, NULL, write_eventcb, NULL, NULL);
	bufferevent_enable(bev, EV_WRITE);
	bufferevent_enable(bev, EV_READ);

	bufferevent_write(bev, MESSAGE, strlen(MESSAGE));

	lasttime = newtime;

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(timeout, &tv);

}

int main(int argc, char **argv)
{
	struct event_base *base;
	struct evconnlistener *listener;
	struct event *signal_event;

	struct sockaddr_in sin = {0};
	base = event_base_new();
	if(!base){
		fprintf(stderr, "cant initialize libevent\n");
		return 1;
	}

	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&sin,
		sizeof(sin));

	if (!listener) {
		fprintf(stderr, "cant create a listener \n");
		return 1;
	}

	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

	if (!signal_event || event_add(signal_event, NULL) < 0) {
		fprintf(stderr, "cant create a signal \n");
		return 1;
	}


	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);

	printf("done\n");
	return 0;
}


static void listener_cb(struct evconnlistener * listener, evutil_socket_t fd,
	struct sockaddr *sa, int socklen, void *user_data)
{
	struct event_base *base = user_data;

	struct event* timeout;
	struct timeval tv;
	int flags;

	flags = EV_PERSIST;

	multievent.base = base;
	multievent.timeout = timeout;
	/* Initialize one event */
	timeout =  event_new(base ,fd, 0, timeout_cb, (void *)&multievent);

	evutil_timerclear(&tv);
	tv.tv_sec = 2;
	event_add(timeout, &tv);

	evutil_gettimeofday(&lasttime, NULL);

}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = user_data;
	struct timeval delay = {1, 0};

	event_base_loopexit(base, &delay);
}

static void write_eventcb(struct bufferevent *bev, void *user_data)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0)
	{
		printf("flushed answer\n");
		bufferevent_free(bev);
	}

}

