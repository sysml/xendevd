#include <xdd/bridge.h>
#include <xdd/iface.h>
#include <xdd/vif.h>
#include <xdd/xs_helper.h>


int vif_hotplug_online(struct xs_handle* xs, const char* xb_path, const char* bridge, const char* vif)
{
    errno = bridge_add_if(bridge, vif);
    if (errno) {
        goto out_err;
    }

    errno = iface_set_up(vif);
    if (errno) {
        goto out_err;
    }

    xs_write_k(xs, "connected", xb_path, "hotplug-status");

    goto out;

out_err:
    /* FIXME: provide an error description */
    xs_write_k(xs, "failure", xb_path, "hotplug-error");
    xs_write_k(xs, "error", xb_path, "hotplug-status");

out:
    return 0;
}

int vif_hotplug_offline(struct xs_handle* xs, const char* xb_path, const char* bridge, const char* vif)
{
    errno = iface_set_down(vif);
    if (errno) {
        return errno;
    }

    errno = bridge_rem_if(bridge, vif);
    if (errno) {
        return errno;
    }

    return 0;
}
