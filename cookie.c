#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "util.h"
#include "header.h"
#include "cookie.h"

CookieEntry *chttp_get_cookie_entry(Header *header, char *key) {
	List *entry = &header->cookie;
	for(ListNode *i = list_begin(entry); i != list_end(entry); i = list_next(i)) {
		CookieEntry *current = (CookieEntry*)i;
		if(stricmp(key, current->key) == 0) {
			return current;
		}
	}
	return NULL;
}

void chttp_add_cookie(Header *header, char *key, char *format, ...) {

	va_list args, argsc;
    va_start(args, format);
    va_copy(argsc, args);

    int len = vsnprintf(NULL, 0, format, args) + 1;
    char val[len + 1];
	vsnprintf(val, len + 1, format, argsc);

	va_end(args);
	va_end(argsc);

	CookieEntry *find_header = chttp_get_cookie_entry(header, key);
	if(find_header) {
		find_header->val = realloc(find_header->val, (strlen(val) + 1) * sizeof(char));
		strcpy(find_header->val, val);
	} else {
		CookieEntry *entry = malloc(sizeof(CookieEntry));
		entry->key = malloc((strlen(key) + 1) * sizeof(char));
		entry->val = malloc((strlen(val) + 1) * sizeof(char));
		strcpy(entry->key, key);
		strcpy(entry->val, val);

		list_insert(list_end(&header->cookie), entry);
	}
}

char *chttp_build_cookie(Header *header) {
	char *result = malloc(sizeof(char) + 1);
	*result = '\0';
	List *entry = &header->cookie;
	for(ListNode *i = list_begin(entry); i != list_end(entry); i = list_next(i)) {
		CookieEntry *current = (CookieEntry*)i;

		char *append = malloc_fmt("Set-Cookie: %s=%s; Expires=Fri, 31 Dec 9999 23:59:59 GMT;\r\n", current->key, current->val);
		result = realloc(result, strlen(result) + strlen(append) + 1);
		strcat(result, append);
		free(append);
	}
	return result;
}

int chttp_parse_cookie(Header *header, char* buf) {
	char *saveptr;
	char *key = strtok_r(buf, "=", &saveptr);
	char *val = strtok_r(NULL, ";", &saveptr);
	while(key != NULL && val != NULL) {
		char key_decoded[strlen(key) + 1];
		uri_decode(key, strlen(key), key_decoded);

		char val_decoded[strlen(val) + 1];
		uri_decode(val, strlen(val), val_decoded);
		chttp_add_cookie(header, trimtrailing(key_decoded), "%s", trimtrailing(val_decoded));
		key = strtok_r(NULL, "=", &saveptr);
		val = strtok_r(NULL, ";", &saveptr);
	}
	return 0;
}

void chttp_free_cookie_entry(CookieEntry *entry) {
	free(entry->key);
	free(entry->val);
	free(entry);
}

void chttp_clean_cookie(Header *header) {
	List *entry = &header->cookie;
	while(!list_empty(entry)) {
		CookieEntry *current = (CookieEntry*)list_remove(list_begin(entry));
		chttp_free_cookie_entry(current);
	}
}