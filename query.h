#ifndef QUERY_H
#define QUERY_H

#include "header.h"

typedef struct _QueryEntry {
	ListNode node;
	char *key;
	char *val;
} QueryEntry;

QueryEntry *get_query_entry(Header *header, char *key);
void add_query(Header *header, char *key, char *format, ...);
char *build_query(Header *header);
int parse_query(Header *header, char* buf);
char *build_query(Header *header);
void free_query_entry(QueryEntry *entry);
void free_all_query_entries(Header *header);

#endif