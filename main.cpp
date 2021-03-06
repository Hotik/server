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
#include <getopt.h>
#include <arpa/inet.h>
#include <cstring>


using namespace std;

static const char* templ = "HTTP/1.0 200 OK\r\n"
		           "Content-length: %d\r\n"
		       	   "Connection: close\r\n"
		       	   "Content-Type: text/html\r\n"
		       	   "\r\n"
		       	   "%s";
		       	   
 static const char *not_found = "HTTP/1.0 404 NOT FOUND\r\n"
 		           	"Content-length: %d\r\n"
 				"Connection: close\r\n"
 				"Content-Type: text/html\r\n"
 				"\r\n"
 				"%s";
static const char *not_found_body = "<html>\n<head>\n<title>Not Found</title>\n</head>\r\n"
 				"<body>\n<p>404 Request file not found.</p>\n</body>\n</html>\r\n";


    char* dir;

bool daemonize(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
		return false;
	if (pid != 0)
		_exit(0);

	//close(STDIN_FILENO);
	//close(STDOUT_FILENO);
	//close(STDERR_FILENO);


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

char *parse_http(char *str)
{
	string res;
	char *tmp;
	tmp = strchr(str, ' ');
	str = tmp + 1;
	tmp = strchr(str, '?');
	if (tmp) {
	    *tmp = '\0';
	} else {
		tmp = strchr(str, ' ');
		*tmp = '\0';
	}
	return str;
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    char buffer[1024];
    size_t nparsed;
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
    	char* s;
       s = parse_http(buffer);
       char* filename = (char*)malloc(100);
       *filename = '\0';
      // strcat(filename, "/");
       strcat(filename, dir);
       strcat(filename, s);
       FILE *f = fopen(filename, "r");
       free(filename);
       if (f) {
       	  char data[1024];
       	  fgets(data, 1024, f);
       	  sprintf(buffer, templ, strlen(data), data);
       	  send(watcher->fd, buffer, strlen(buffer), MSG_NOSIGNAL);
       }
       else {
       	     
       	     sprintf(buffer, not_found, strlen(not_found_body), not_found_body);
       	     send(watcher->fd, buffer, strlen(buffer), MSG_NOSIGNAL);

       }
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

void get_params(int argc, char **argv, char **addr, int *port, char **dir)
{
	int opt;
	
	while ((opt = getopt(argc, argv, "h:p:d:")) != -1)
	{
		switch (opt) {
			case 'h':
				*addr = optarg;
				break;
			case 'p':
				*port = atoi(optarg);
				break;
			case 'd':
				*dir  = optarg;
				break;
			default:
				cout << "Error, wrong args";
		}
	}
}



int main(int argc, char *argv[])
{
    char* adr = NULL;
    int port;
    if (!daemonize())
        cout << "failed become daemon";
    get_params(argc, argv, &adr, &port, &dir);
    cout << adr;
        
    struct ev_loop *loop = ev_default_loop(0);
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(adr, &addr.sin_addr);
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
