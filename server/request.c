#include "request.h"


#include "../log/log.h"

request_method_t parse_method(char *method);

int validate_version(char *version);

int parse_req(requestT *req, char *buff) {
    char *saveptr = NULL;

    char *http_query = strtok_r(buff, "\n", &saveptr);
    if (http_query == NULL) {
        logError(ERR_FSTR, "failed parse http query string", strerror(errno));
        return -1;
    }
    logInfo(http_query);

    char *http_method = strtok_r(http_query, " ", &saveptr);
    if (http_method == NULL) {
        logError(ERR_FSTR, "failed parse http method", strerror(errno));
        return -1;
    }
    req->method = parse_method(http_method);
    if (req->method == BAD) {
        return 0;
    }
    logDebug("method: %s - %d", http_method, req->method);

    char *http_url = strtok_r(NULL, " ", &saveptr);
    if (http_url == NULL) {
        logError(ERR_FSTR, "failed parse url", strerror(errno));
        return -1;
    }
    if (strcmp(http_url, "/") == 0)
        strcpy(req->url, "index.html");
    else
        strcpy(req->url, http_url + 1);
    logDebug("url: %s ", req->url);

    char *http_version = strtok_r(NULL, "\r", &saveptr);
    if (http_version == NULL) {
        logError(ERR_FSTR, "failed http version", strerror(errno));
        return -1;
    }
    if (validate_version(http_version) == 0) {
        logError("unsupported http version");
        return -1;
    }
    logDebug("version: %s ", http_version);

    return 0;
}

request_method_t parse_method(char *method) {
    if (strcmp(GET_STR, method) == 0) {
        return GET;
    }
    if (strcmp(HEAD_STR, method) == 0) {
        return HEAD;
    }
    return BAD;
}

int validate_version(char *version) {
    if (strcmp(HTTP11_STR, version) == 0 || strcmp(HTTP10_STR, version) == 0) {
        return 1;
    }
    return 0;
}
