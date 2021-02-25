#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <netinet/in.h>
#endif

#include "util.h"
#include "chiphttp.h"
#include "query.h"
#include "cookie.h"

Server *new_server(int port) {
	signal(SIGPIPE, SIG_IGN);

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(sock == -1) { 
        error("Unable to create server");
    }

	struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
	
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(const char){1}, sizeof(int)) < 0) {
		error("setsockopt(SO_REUSEADDR) failed");
	}
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
		error("setsockopt(SO_RCVTIMEO) failed");
	}

	if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
		error("setsockopt(SO_SNDTIMEO) failed");
	}

	SockAddrIn server_addr;
	server_addr.sin_family      = AF_INET; 
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_addr.sin_port        = htons(port); 

    if((bind(sock, (SockAddr*)&server_addr, sizeof(server_addr))) != 0) { 
        error("socket bind failed..."); 
    }

    if((listen(sock, 5)) != 0) {
    	error("listen failed..."); 
    }

    Server *server = malloc(sizeof(Server));
    server->fd     = sock;
    pthread_mutex_init(&server->lock, NULL);

    return server;
}

void new_worker(Server *server, void *(*callback) (void *)) {
	while(1) {
		SockAddrIn client_addr;
		socklen_t client_length = sizeof(client_addr);

		int clientfd = accept(server->fd, (SockAddr*)&client_addr, &client_length);
		Client *client = malloc(sizeof(Client));
		if(client) {
			client->fd              = clientfd;
			client->is_header_sent  = false;
			client->state           = CONNECTED;
			client->callback        = callback;
			client->lock            = server->lock;
			client->data            = server->data;

			pthread_t thread;
			pthread_create(&thread, NULL, worker_thread, (void*)client);
			pthread_detach(thread);
		}	
	}
}

void *worker_thread(void *args) {
	Client *client = (Client*)args;
	char header_buf[8192 * 2];

	client->request_header  = new_header();
	client->response_header = new_header();

	while(1) {
		header_buf[0] = '\0';
		int readed = 0;
		char bit;
		while(strstr(header_buf, "\r\n\r\n") == NULL) {
			if((readed += client_read(client, &bit, sizeof(bit))) < 1) {
				client_close(client);
				break;
			}
			if(readed >= sizeof(header_buf)) {
				// Buffer Overflow
				break;
			}
			strncat(header_buf, &bit, 1);
		}
		free_all_query_entries(client->request_header);
		free_all_query_entries(client->response_header);
		free_all_header_entries(client->request_header);
		free_all_header_entries(client->response_header);
		free_all_cookie_entries(client->request_header);
		free_all_cookie_entries(client->response_header);
		if((parse_header(client->request_header, (char*)&header_buf)) == 0) {

			add_header(client->response_header, "Connection", "Keep-Alive");
			add_header(client->response_header, "Content-Type", "text/html");
			add_header(client->response_header, "Keep-Alive", "timeout=5, max=1000");
			add_header(client->response_header, "Server", "ColdChip Web Server");

			client->is_header_sent = false;
			if(client->request_header->method == METHOD_POST) {
				HeaderEntry *length_entry = get_header_entry(client->request_header, "Content-Length");
				if(length_entry) {
					long long length = atoi(length_entry->val);
					if(length > 0 && length < MAX_POST_LENGTH) {
						char post_data[MAX_POST_LENGTH];
						int readed = 0;
						while(readed < length) {
							if((readed += client_read(client, (char*)&post_data, sizeof(post_data) - 1)) < 1) {
								client_close(client);
								break;
							}
						}
						post_data[sizeof(post_data) - 1] = '\0';
						if((parse_query(client->request_header, (char*)&post_data)) == 0) {

						}
					}
				}
			}

			client->callback(client);
		} else {
			client_close(client);
			break;
		}
		
	}
	free_all_cookie_entries(client->request_header);
	free_all_cookie_entries(client->response_header);
	free_all_query_entries(client->request_header);
	free_all_query_entries(client->response_header);
	free_header(client->request_header);
	free_header(client->response_header);

	free(client);

	return NULL;
}

bool is_connected(Client *client) {
	return client->state == CONNECTED;
}

bool is_disconnected(Client *client) {
	return client->state == DISCONNECTED;
}

int client_read(Client *client, void *buf, int len) {
	int res = 0;
	if(is_connected(client)) {
		res = recv(client->fd, buf, len, 0);
	} else {
		res = -1;
	}
	return res;
}

int client_write_header(Client *client) {
	int res;
	if(is_connected(client)) {

		char *header_string = build_header(client->response_header);
		res = send(client->fd, header_string, strlen(header_string), 0);
		free(header_string);
	} else {
		res = -1;
	}
	return res;
}

int client_write(Client *client, void *buf, int len) {
	int res = 1;
	if(is_connected(client)) {
		if(client->is_header_sent == false) {
			client->is_header_sent = true;
			res = client_write_header(client);
		}
		if(res > 0) {
			res = send(client->fd, buf, len, 0);
		}
	} else {
		res = -1;
	}
	return res;
}

void client_close(Client *client) {
	if(is_connected(client)) {
		client->state = DISCONNECTED;
		close(client->fd);
	}
}

void serve_root(Client *client, char *root) {
	char *path = client->request_header->path;

	if(strstr(path, "..") == NULL && strstr(path, "./.") == NULL) {
		char *new_path = malloc_fmt("%s/%s", root, path);

		struct stat path_stat;
    	stat(new_path, &path_stat);

    	if(!S_ISREG(path_stat.st_mode)) {
    		char *old_new_path = new_path;
    		new_path = malloc_fmt("%s/%s", new_path, "index.html");
    		free(old_new_path);
    	}
    	
    	stat(new_path, &path_stat);

    	FILE *fp = fopen(new_path, "rb");
		if(fp && S_ISREG(path_stat.st_mode)) {
			fseek(fp, 0, SEEK_END);
			long fsize = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char *mime = get_mime_type(new_path);

			set_status(client->response_header, 200);
			add_header(client->response_header, "Content-Type", "%s", mime);
			add_header(client->response_header, "Content-Length", "%i", fsize);

			char buf[8192 * 4];
			while(!feof(fp)) {
				int read = fread(buf, sizeof(char), sizeof(buf), fp);
				if(client_write(client, buf, read) < 1) {
					break;
				}
			}

			fclose(fp);
			free(mime);
		} else {
			char data[] = "Not Found";
			set_status(client->response_header, 404);
			add_header(client->response_header, "Content-Type", "text/html");
			add_header(client->response_header, "Content-Length", "%i", strlen(data));
			client_write(client, data, strlen(data));
		}
		free(new_path);
	} else {
		char data[] = "500 Internal Error";
		set_status(client->response_header, 500);
		add_header(client->response_header, "Content-Type", "text/html");
		add_header(client->response_header, "Content-Length", "%i", strlen(data));
		client_write(client, data, strlen(data));
	}
}

void console_log(char *format, ...) {
	va_list args;
    va_start(args, format);

	char fmt[1000];
	snprintf(fmt, sizeof(fmt), "\033[0;32m[ChipHTTP] %s\033[0m\n", format);
	vprintf(fmt, args);
    
    va_end(args);
}

void warning(char *format, ...) {
	va_list args;
    va_start(args, format);

	char fmt[1000];
	snprintf(fmt, sizeof(fmt), "\033[0;31m[ChipHTTP] %s\033[0m\n", format);
	vprintf(fmt, args);
    
    va_end(args);
}

void error(char *format, ...) {
	va_list args;
    va_start(args, format);

	char fmt[1000];
	snprintf(fmt, sizeof(fmt), "\033[0;31m[ChipHTTP] %s\033[0m\n", format);
	vprintf(fmt, args);
    
    va_end(args);
	exit(1);
}

char *read_file_into_buffer(char *file, long *size) {
	FILE *infp = fopen(file, "rb");
    if (!infp) {
    	return NULL;
    }
    fseek(infp, 0, SEEK_END);
	long fsize = ftell(infp);
	char *p = malloc(fsize + 1);
	fseek(infp, 0, SEEK_SET);

	if(fread((char*)p, sizeof(char), fsize, infp)) {}

	fclose(infp);

	*size = fsize;

	return p;
}

char *get_mime_type(char *r_ext) {
	char *e_ext = strrchr(r_ext, '.');
	if(!e_ext) {
		e_ext = r_ext;
	}
	e_ext++;
	
	if(stricmp(e_ext, "js") == 0) {
		return strdup("application/javascript");
	} else if(stricmp(e_ext, "html") == 0) {
		return strdup("text/html");
	} else if(stricmp(e_ext, "css") == 0) {
		return strdup("text/css");
	} else if(stricmp(e_ext, "png") == 0) {
		return strdup("image/png");
	} else if(stricmp(e_ext, "jpg") == 0) {
		return strdup("image/jpeg");
	} else if(stricmp(e_ext, "jpeg") == 0) {
		return strdup("image/jpeg");
	} else if(stricmp(e_ext, "svg") == 0) {
		return strdup("image/svg+xml");
	} else if(stricmp(e_ext, "mp3") == 0) {
		return strdup("audio/mp3");
	} else if(stricmp(e_ext, "mp4") == 0) {
		return strdup("video/mp4");
	} else if(stricmp(e_ext, "mov") == 0) {
		return strdup("video/quicktime");
	} else {
		return strdup("application/octet-stream");
	}
	
}