#pragma once

#include <string.h>
#include <errno.h>

#define GET_STR "GET"
#define HEAD_STR "HEAD"

#define HTTP11_STR "HTTP/1.1"
#define HTTP10_STR "HTTP/1.0"

#define URL_LEN 128

typedef enum requestMethod {
    BAD, GET, HEAD
} request_method_t;

typedef struct request {
    request_method_t method;
    char url[URL_LEN];
} requestT;

int parse_req(requestT *req, char *buff);
