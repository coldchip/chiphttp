#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "util.h"
#include "query.h"
#include "list.h"

QueryEntry *chttp_get_query_entry(Header *header, char *key) {
	List *entry = &header->query;
	for(ListNode *i = list_begin(entry); i != list_end(entry); i = list_next(i)) {
		QueryEntry *current = (QueryEntry*)i;
		if(stricmp(key, current->key) == 0) {
			return current;
		}
	}
	return NULL;
}

void chttp_add_query(Header *header, char *key, char *format, ...) {

	va_list args, argsc;
    va_start(args, format);
    va_copy(argsc, args);

    int len = vsnprintf(NULL, 0, format, args) + 1;
    char val[len + 1];
	vsnprintf(val, len + 1, format, argsc);

	va_end(args);
	va_end(argsc);

	QueryEntry *find_header = chttp_get_query_entry(header, key);
	if(find_header) {
		find_header->val = realloc(find_header->val, (strlen(val) + 1) * sizeof(char));
		strcpy(find_header->val, val);
	} else {
		QueryEntry *entry = malloc(sizeof(QueryEntry));
		entry->key = malloc((strlen(key) + 1) * sizeof(char));
		entry->val = malloc((strlen(val) + 1) * sizeof(char));
		strcpy(entry->key, key);
		strcpy(entry->val, val);

		list_insert(list_end(&header->query), entry);
	}
}

char *chttp_build_query(Header *header) {
	char *result = malloc(sizeof(char) + 1);
	*result = '\0';
	List *entry = &header->query;
	for(ListNode *i = list_begin(entry); i != list_end(entry); i = list_next(i)) {
		QueryEntry *current = (QueryEntry*)i;

		char *append = malloc_fmt("%s=%s& ", current->key, current->val);
		result = realloc(result, strlen(result) + strlen(append) + 1);
		strcat(result, append);
		free(append);
	}
	return result;
}

int chttp_parse_query(Header *header, char* buf) {
	char *saveptr;
	char *key = strtok_r(buf, "=", &saveptr);
	char *val = strtok_r(NULL, "&", &saveptr);
	while(key != NULL && val != NULL) {

		char key_decoded[strlen(key) + 1];
		uri_decode(key, strlen(key), key_decoded);

		char val_decoded[strlen(val) + 1];
		uri_decode(val, strlen(val), val_decoded);

		chttp_add_query(header, key_decoded, "%s", val_decoded);
		key = strtok_r(NULL, "=", &saveptr);
		val = strtok_r(NULL, "&", &saveptr);
	}
	return 0;
}

void chttp_free_query_entry(QueryEntry *entry) {
	free(entry->key);
	free(entry->val);
	free(entry);
}

void chttp_clean_query(Header *header) {
	List *entry = &header->query;
	while(!list_empty(entry)) {
		QueryEntry *current = (QueryEntry*)list_remove(list_begin(entry));
		chttp_free_query_entry(current);
	}
}