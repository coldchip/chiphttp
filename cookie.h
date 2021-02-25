#ifndef COOKIE_H
#define COOKIE_H

#include "header.h"

typedef struct _CookieEntry {
  ListNode node;
  char *key;
  char *val;
  int expiry;
} CookieEntry;

CookieEntry *chttp_get_cookie_entry(Header *header, char *key);
void chttp_add_cookie(Header *header, char *key, char *format, ...);
char *chttp_build_cookie(Header *header);
int chttp_parse_cookie(Header *header, char* buf);
char *chttp_build_cookie(Header *header);
void chttp_free_cookie_entry(CookieEntry *entry);
void chttp_clean_cookie(Header *header);

#endif