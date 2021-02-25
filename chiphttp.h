#ifndef CHIPHTTP_H
#define CHIPHTTP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <netinet/in.h>
#endif

#include "list.h"
#include "header.h"

#define MAX_POST_LENGTH 8192

typedef struct sockaddr_in SockAddrIn;
typedef struct sockaddr SockAddr;

typedef enum {
	CONNECTED,
	DISCONNECTED
} ClientState;

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
	ClientState state;
	SockAddrIn addr;
	void *(*callback) (void *);
	pthread_mutex_t lock;
    void *data;
} Client;

Server *new_server(int port);
void new_worker(Server *server, void *(*callback) (void *));
void *worker_thread(void *args);
void free_server();
int client_write_header(Client *client);
bool is_connected(Client *client);
bool is_disconnected(Client *client);
int client_read(Client *client, void *buf, int len);
int client_write(Client *client, void *buf, int len);
void client_close(Client *client);
void serve_root(Client *client, char *root);
void console_log(char *format, ...);
void warning(char *format, ...);
void error(char *format, ...);
char *read_file_into_buffer(char *file, long *size);
char *get_mime_type(char *r_ext);

#endif