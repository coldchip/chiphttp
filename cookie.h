#ifndef COOKIE_H
#define COOKIE_H

#include "header.h"

typedef struct _CookieEntry {
  ListNode node;
  char *key;
  char *val;
  int expiry;
} CookieEntry;

CookieEntry *get_cookie_entry(Header *header, char *key);
void add_cookie(Header *header, char *key, char *format, ...);
char *build_cookie(Header *header);
int parse_cookie(Header *header, char* buf);
char *build_cookie(Header *header);
void free_cookie_entry(CookieEntry *entry);
void free_all_cookie_entries(Header *header);

#endif