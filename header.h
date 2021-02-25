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

Header *new_header();
void set_status(Header *header, int status);
HeaderEntry *get_header_entry(Header *header, char *key);
void add_header(Header *header, char *key, char *format, ...);
char *build_header(Header *header);
bool is_break(char *data);
char *trimtrailing(char *str);
int parse_header(Header *header, char* buf);
void free_header_entry(HeaderEntry *entry);
void free_all_header_entries(Header *entry);
void free_header(Header *header);

#endif