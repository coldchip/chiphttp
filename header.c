#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "util.h"
#include "cookie.h"
#include "query.h"

Header *new_header() {
	Header *header = malloc(sizeof(Header));
	list_clear(&header->entry);
	list_clear(&header->query);
	list_clear(&header->cookie);
    header->status = 200;
	return header;
}

void set_status(Header *header, int status) {
	header->status = status;
}

HeaderEntry *get_header_entry(Header *header, char *key) {
	List *entry = &header->entry;
	for(ListNode *i = list_begin(entry); i != list_end(entry); i = list_next(i)) {
		HeaderEntry *current = (HeaderEntry*)i;
		if(stricmp(key, current->key) == 0) {
			return current;
		}
	}
	return NULL;
}

void add_header(Header *header, char *key, char *format, ...) {

	va_list args, argsc;
    va_start(args, format);
    va_copy(argsc, args);

    int len = vsnprintf(NULL, 0, format, args) + 1;
    char val[len + 1];
	vsnprintf(val, len + 1, format, argsc);

	va_end(args);
	va_end(argsc);

	HeaderEntry *find_header = get_header_entry(header, key);
	if(find_header) {
		find_header->val = realloc(find_header->val, (strlen(val) + 1) * sizeof(char));
		strcpy(find_header->val, val);
	} else {
		HeaderEntry *entry = malloc(sizeof(HeaderEntry));
		entry->key = malloc((strlen(key) + 1) * sizeof(char));
		entry->val = malloc((strlen(val) + 1) * sizeof(char));
		strcpy(entry->key, key);
		strcpy(entry->val, val);

		list_insert(list_end(&header->entry), entry);
	}
}

char *build_header(Header *header) {
	// Inject Status Code and Protocol
	char status[128];
	if(header->status == 200) {
		strcpy(status, "HTTP/1.1 200 OK\r\n");
	} else if(header->status == 206) {
		strcpy(status, "HTTP/1.1 206 Partial Content\r\n");
	} else if(header->status == 302) {
		strcpy(status, "HTTP/1.1 302 Found\r\n");
	} else if(header->status == 404) {
		strcpy(status, "HTTP/1.1 404 Not Found\r\n");
	} else if(header->status == 401) {
		strcpy(status, "HTTP/1.1 401 Unauthorized\r\n");
	} else if(header->status == 416) {
		strcpy(status, "HTTP/1.1 416 Requested Range Not Satisfiable\r\n");
	} else {
		strcpy(status, "HTTP/1.1 200 OK\r\n");
	}
	char *result = malloc(strlen(status) + 1);
	*result = '\0';
	strcpy(result, status);

	// Inject Headers
	List *entry = &header->entry;
	for(ListNode *i = list_begin(entry); i != list_end(entry); i = list_next(i)) {
		HeaderEntry *current = (HeaderEntry*)i;

		char *line = malloc_fmt("%s: %s\r\n", current->key, current->val);
		result = realloc(result, strlen(result) + strlen(line) + 1);
		strcat(result, line);
		free(line);
	}

	// Inject Set-Cookie if Cookies Buffer > 0
	if(list_size(&header->cookie) > 0) {
		char *cookie = build_cookie(header);
		result = realloc(result, strlen(result) + strlen(cookie) + 1);
		strcat(result, cookie);
		free(cookie);
	}
	// Insert Last Line Break
	result = realloc(result, strlen(result) + 2 + 1);
	strcat(result, "\r\n");
	return result;
}

bool is_break(char *data) {
	return strncmp(data, "\r\n", 2) == 0;
}

char *trimtrailing(char *str) {
	while(*str == ' ') {
		str++;
	}
	if(*str == 0) {
		return str;
	}
	return str;
}

int parse_header(Header *header, char* buf) {
	int header_index = 0;
	char *p = buf;
	while(1) {
		if(*p == '\0') {
			return -1;
		} else if(is_break(p)) {
			break;
		} else {
			char *start = p;
			while(!is_break(p)) {
				if(*p == '\0') {
					return -1;
				}
				p++;
			}

			char k[p - start + 1];
			strncpy(k, start, p - start);
			*(((char*)&k) + (p - start)) = '\0';

			if(header_index == 0) {
				char *saveptr;
				char *method     = strtok_r(k, " ", &saveptr);
				char *path_query = strtok_r(NULL, " ", &saveptr);
				char *proto      = strtok_r(NULL, "", &saveptr);
				if(method && path_query && proto) {
					char *saveptr2;
					char *path  = strtok_r(path_query, "?", &saveptr2);
					char *query = strtok_r(NULL, "", &saveptr2);
					if(query) {
						if(parse_query(header, query) != 0) {
							return -1;
						}
					}
					if(strcmp(method, "GET") == 0) {
						header->method = METHOD_GET;
					} else if(strcmp(method, "POST") == 0) {
						header->method = METHOD_POST;
					} else if(strcmp(method, "PUT") == 0) {
						header->method = METHOD_PUT;
					} else {
						return -1;
					}
					strncpy(header->path, path, MAX_URL_LENGTH - 1);

				} else {
					return -1;
				}
			} else {
				char *saveptr;
				char *key = strtok_r(k, ":", &saveptr);
				char *val = strtok_r(NULL, "", &saveptr);
				if(key && val) {
					add_header(header, trimtrailing(key), "%s", trimtrailing(val));
					if(stricmp(key, "cookie") == 0) {
						if(parse_cookie(header, val) != 0) {
							return -1;
						}
					}
				} else {
					return -1;
				}
			}
			p += 2;
			header_index++;
		}
	}

	return 0;
}

void free_header_entry(HeaderEntry *entry) {
	free(entry->key);
	free(entry->val);
	free(entry);
}

void free_all_header_entries(Header *header) {
	List *entry = &header->entry;
	while(!list_empty(entry)) {
		HeaderEntry *current = (HeaderEntry*)list_remove(list_begin(entry));
		free_header_entry(current);
	}
}

void free_header(Header *header) {
	free_all_header_entries(header);
	free(header);
}