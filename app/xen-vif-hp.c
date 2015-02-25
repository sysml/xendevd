#include <xdd/bridge.h>
#include <xdd/vif.h>
#include <xdd/xs_helper.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xenstore.h>


enum operation {
    ONLINE  ,
    OFFLINE ,
};

static void print_usage(char* cmd)
{
    printf("Usage: %s <online|offline>\n", cmd);
}


int main(int argc, char** argv)
{
    enum operation op;
    char* vif = NULL;
    char* bridge = NULL;
    char* xb_path = NULL;

    struct xs_handle *xs = NULL;

    /* Get parameters */
    if (argc < 2) {
        print_usage(argv[0]);

        errno = EINVAL;
        goto out;
    }

    if (strcmp(argv[1], "online") == 0) {
        op = ONLINE;
    } else if (strcmp(argv[1], "offline") == 0) {
        op = OFFLINE;
    } else {
        print_usage(argv[0]);
        errno = EINVAL;
        goto out;
    }

    vif = getenv("vif");
    if (vif == NULL) {
        errno = EINVAL;
        goto out;
    }

    xb_path = getenv("XENBUS_PATH");
    if (xb_path == NULL) {
        errno = EINVAL;
        goto out;
    }


    /* Execute */
    xs = xs_open(XS_OPEN_READONLY);
    if (xs == NULL) {
        goto out;
    }

    bridge = xs_read_k(xs, xb_path, "bridge");
    if (bridge == NULL) {
        xs_write_k(xs, "Unable to read bridge from xenstore", xb_path, "hotplug-error");
        xs_write_k(xs, "error", xb_path, "hotplug-status");
        goto out;
    }

    switch (op) {
        case ONLINE:
            errno = vif_hotplug_online(xs, xb_path, bridge, vif);
            break;
        case OFFLINE:
            errno = vif_hotplug_offline(xs, xb_path, bridge, vif);
            break;
    }

out:
    if (bridge) {
        free(bridge);
    }

    if (xs) {
        xs_close(xs);
    }

    return errno;
}
