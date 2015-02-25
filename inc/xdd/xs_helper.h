#ifndef __XDD__XS_HELPER__HH__
#define __XDD__XS_HELPER__HH__

#define _GNU_SOURCE

#include <stddef.h>
#include <xenstore.h>


char* xs_read_k(struct xs_handle* xs, char* base_path, char* key);
int xs_write_k(struct xs_handle* xs, char* value, char* base_path, char* key);

#endif /* __XDD_XS_HELPER__HH__ */
