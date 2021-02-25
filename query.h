#ifndef QUERY_H
#define QUERY_H

#include "header.h"

typedef struct _QueryEntry {
	ListNode node;
	char *key;
	char *val;
} QueryEntry;

QueryEntry *chttp_get_query_entry(Header *header, char *key);
void chttp_add_query(Header *header, char *key, char *format, ...);
char *chttp_build_query(Header *header);
int chttp_parse_query(Header *header, char* buf);
char *chttp_build_query(Header *header);
void chttp_free_query_entry(QueryEntry *entry);
void chttp_clean_query(Header *header);

#endif