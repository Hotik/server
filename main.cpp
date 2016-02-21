#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <list>
#include <netinet/in.h>
#include <string>
#include <stdio.h>
#include <ev.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

bool daemonize(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		return false;
	if (pid != 0)
		_exit(0);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);


	if (open("/dev/null", O_WRONLY) != 0)
		return false;
	if (dup2(0, STDERR_FILENO) != STDERR_FILENO)
		return false;
	close(0);

	if (setsid() == (pid_t)-1)
	        return false;
	if (chdir("/") != 0)
		return false;

	umask(0);
	return true;
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    char buffer[1024];
    ssize_t r = recv(watcher->fd, buffer, 1024, MSG_NOSIGNAL);
    if (r < 0)
        return;
    else if (r == 0)
    {
        ev_io_stop(loop, watcher);
        free(watcher);
        return;
    }
    else
    {
        send(watcher->fd, buffer, r, MSG_NOSIGNAL);

    }
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    int client_sd = accept(watcher->fd, 0, 0);

    struct ev_io *w_client = (struct ev_io*)
                                    malloc(sizeof(struct ev_io));
    ev_io_init(w_client, read_cb, client_sd, EV_READ);
    ev_io_start(loop, w_client);
}

int main(int argc, char *argv[])
{
    if (!daemonize())
        cout << "failed become daemon";
    struct ev_loop *loop = ev_default_loop(0);
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sd, (struct sockaddr *)&addr, sizeof(addr));

    listen(sd, SOMAXCONN);
    struct ev_io w_accept;
    ev_io_init(&w_accept, accept_cb, sd, EV_READ);
    ev_io_start(loop, &w_accept);

    while(1)
        ev_loop(loop, 0);
    
    sleep(60);
    return 0;
}
