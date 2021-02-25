#ifndef HEADER_H
#define HEADER_H

#include "list.h"
#include <stdbool.h>

#define MAX_URL_LENGTH 2048

typedef enum {
  METHOD_POST,
  METHOD_GET,
  METHOD_PUT
} Method;

typedef enum {
  HTTP_1_0,
  HTTP_1_1
} Proto;

typedef struct _HeaderEntry {
  ListNode node;
  char *key;
  char *val;
} HeaderEntry;

typedef struct _Header {
  int status;
  Method method;
  char path[MAX_URL_LENGTH];
  Proto proto;
  
  List entry;
  List query;
  List cookie;
} Header;

Header *chttp_new_header();
void chttp_set_status(Header *header, int status);
HeaderEntry *chttp_get_header_entry(Header *header, char *key);
void chttp_add_header(Header *header, char *key, char *format, ...);
char *chttp_build_header(Header *header);
int chttp_parse_header(Header *header, char* buf);
void chttp_free_header_entry(HeaderEntry *entry);
void chttp_clean_header(Header *entry);
void chttp_free_header(Header *header);

#endif