#ifndef CHIPHTTP_H
#define CHIPHTTP_H

#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <netinet/in.h>
#include "list.h"
#include "header.h"

typedef struct sockaddr_in SockAddrIn;
typedef struct sockaddr SockAddr;

typedef struct _Server {
    int fd;
    pthread_mutex_t lock;
    void *data;
} Server;

typedef struct _Client {
	struct _Server *server;
	int fd;
	bool is_header_sent;
	Header *request_header;
	Header *response_header;
	SockAddrIn addr;
	void *(*callback) (void *);
	pthread_mutex_t lock;
    void *data;
} Client;

Server *chttp_new(int port);
void chttp_run(Server *server, void *(*callback) (void *));
void *chttp_thread(void *args);
int chttp_client_read(Client *client, void *buf, int len);
int chttp_client_write(Client *client, void *buf, int len);
void chttp_client_close(Client *client);

#endif