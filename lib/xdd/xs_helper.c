#include <xdd/xs_helper.h>

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xenstore.h>


char* xs_read_k(struct xs_handle* xs, const char* base_path, const char* key)
{
    char* path;
    char* value;
    unsigned int len;

    if (asprintf(&path, "%s/%s", base_path, key) < 0) {
        return NULL;
    }

    value = (char*) xs_read(xs, XBT_NULL, path, &len);

    free(path);

    return value;
}

int xs_write_k(struct xs_handle* xs, const char* value, const char* base_path, const char* key)
{
    bool ret;
    char* path;

    if (asprintf(&path, "%s/%s", base_path, key) < 0) {
        return -1;
    }

    ret = xs_write(xs, XBT_NULL, path, value, strlen(value));

    free(path);

    return ret ? 0 : -1;
}
