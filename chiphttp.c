#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include "util.h"
#include "chiphttp.h"
#include "query.h"
#include "cookie.h"

Server *chttp_new(int port) {
	signal(SIGPIPE, SIG_IGN);

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(sock == -1) { 
        return NULL;
    }

	struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
	
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(const char){1}, sizeof(int)) < 0) {
		return NULL;
	}
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
		return NULL;
	}

	if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
		return NULL;
	}

	SockAddrIn server_addr;
	server_addr.sin_family      = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port        = htons(port); 

    if((bind(sock, (SockAddr*)&server_addr, sizeof(server_addr))) != 0) { 
        return NULL;
    }

    if((listen(sock, 5)) != 0) {
    	return NULL;
    }

    Server *server = malloc(sizeof(Server));
    server->fd     = sock;
    pthread_mutex_init(&server->lock, NULL);

    return server;
}

void chttp_run(Server *server, void *(*callback) (void *)) {
	while(1) {
		SockAddrIn client_addr;
		socklen_t client_length = sizeof(client_addr);

		int clientfd = accept(server->fd, (SockAddr*)&client_addr, &client_length);
		Client *client = malloc(sizeof(Client));
		if(client) {
			client->fd              = clientfd;
			client->is_header_sent  = false;
			client->callback        = callback;
			client->lock            = server->lock;
			client->data            = server->data;

			pthread_t thread;
			pthread_create(&thread, NULL, chttp_thread, (void*)client);
			pthread_detach(thread);
		}	
	}
}

void *chttp_thread(void *args) {
	Client *client = (Client*)args;
	char header_buf[8192 * 2];

	client->request_header  = chttp_new_header();
	client->response_header = chttp_new_header();

	while(1) {
		header_buf[0] = '\0';
		int readed = 0;
		char bit;
		while(strstr(header_buf, "\r\n\r\n") == NULL) {
			if((readed += chttp_client_read(client, &bit, sizeof(bit))) < 1) {
				goto chttp_client_cleanup;
			}
			if(readed >= sizeof(header_buf)) {
				goto chttp_client_cleanup;
			}
			strncat(header_buf, &bit, 1);
		}
		chttp_clean_header(client->request_header);
		chttp_clean_header(client->response_header);
		if((chttp_parse_header(client->request_header, (char*)&header_buf)) == 0) {
			chttp_add_header(client->response_header, "Connection", "Keep-Alive");
			chttp_add_header(client->response_header, "Content-Type", "text/html");
			chttp_add_header(client->response_header, "Keep-Alive", "timeout=5, max=1000");
			chttp_add_header(client->response_header, "Server", "ColdChip Web Server");

			client->is_header_sent = false;
			client->callback(client);
		}
		
	}

	chttp_client_cleanup:

	chttp_free_header(client->request_header);
	chttp_free_header(client->response_header);

	chttp_client_close(client);

	free(client);

	return NULL;
}

int chttp_client_read(Client *client, void *buf, int len) {
	return recv(client->fd, buf, len, 0);
}

int chttp_client_write(Client *client, void *buf, int len) {
	int res = 0;
	if(client->is_header_sent == false) {
		client->is_header_sent = true;
		char *header_string = chttp_build_header(client->response_header);
		res = send(client->fd, header_string, strlen(header_string), 0);
		free(header_string);
	}
	if(res > 0) {
		res = send(client->fd, buf, len, 0);
	}
	return res;
}

void chttp_client_close(Client *client) {
	close(client->fd);
}