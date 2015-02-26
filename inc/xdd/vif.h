#ifndef __XDD__VIF__HH__
#define __XDD__VIF__HH__

#define _GNU_SOURCE

#include <stddef.h>
#include <xenstore.h>


int vif_hotplug_online(struct xs_handle* xs, const char* xb_path, const char* bridge, const char* vif);
int vif_hotplug_offline(struct xs_handle* xs, const char* xb_path, const char* bridge, const char* vif);

#endif /* __XDD_VIF_HH__ */
