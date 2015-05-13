#include <xdd/vbd.h>
#include <xdd/xs_helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


int vbd_phy_hotplug_online(struct xs_handle* xs, const char* xb_path, const char* device)
{
    char* dev_id;
    struct stat st;
    char* err_msg = NULL;

    if (stat(device, &st)) {
        if (errno == ENOENT) {
            asprintf(&err_msg, "%s does not exist.", device);
        } else {
            asprintf(&err_msg, "stat(%s) returned %d.", device, errno);
        }
        goto out_err;
    }

    /* FIXME: Check if dev is block device */
    if (!S_ISBLK(st.st_mode)) {
        asprintf(&err_msg, "%s is not a block device.", device);
        goto out_err;
    }

    /* FIXME: Check device sharing */

    asprintf(&dev_id, "%x:%x", major(st.st_rdev), minor(st.st_rdev));
    xs_write_k(xs, dev_id, xb_path, "physical-device");
    free(dev_id);

    goto out;

out_err:
    xs_write_k(xs, err_msg, xb_path, "hotplug-error");
    xs_write_k(xs, "error", xb_path, "hotplug-status");
    free(err_msg);

out:
    return 0;
}
