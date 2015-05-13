#ifndef __XDD__VBD__HH__
#define __XDD__VBD__HH__

#define _GNU_SOURCE

#include <stddef.h>
#include <xenstore.h>


int vbd_phy_hotplug_online(struct xs_handle* xs, const char* xb_path, const char* device);

#endif /* __XDD_VBD_HH__ */
