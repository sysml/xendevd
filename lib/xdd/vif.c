/*
 * xendevd
 *
 * Authors: Filipe Manco <filipe.manco@neclab.eu>
 *
 *
 * Copyright (c) 2017, NEC Europe Ltd., NEC Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <xdd/bridge.h>
#include <xdd/iface.h>
#include <xdd/vif.h>
#include <xdd/xs_helper.h>

#include <stdlib.h>


int vif_hotplug_online_xs(struct xs_handle* xs, const char* xb_path, const char* vif)
{
    int ret = 0;
    char* bridge = NULL;
    char* err_str = NULL;

    bridge = xs_read_k(xs, xb_path, "bridge");
    if (bridge == NULL) {
        ret = EINVAL;
        err_str = "Unable to read bridge from xenstore";
        goto out_err;
    }

    ret = bridge_add_if(bridge, vif);
    if (ret) {
        err_str = "Fail to add interface to bridge";
        goto out_err;
    }

    ret = iface_set_up(vif);
    if (ret) {
        err_str = "Fail to set interface up";
        goto out_err;
    }

    xs_write_k(xs, "connected", xb_path, "hotplug-status");

    free(bridge);

    return 0;

out_err:
    if (bridge) {
        free(bridge);
    }

    xs_write_k(xs, err_str, xb_path, "hotplug-error");
    xs_write_k(xs, "error", xb_path, "hotplug-status");

    return ret;
}

int vif_hotplug_offline_xs(struct xs_handle* xs, const char* xb_path, const char* vif)
{
    int ret = 0;
    char* bridge = NULL;

    bridge = xs_read_k(xs, xb_path, "bridge");
    if (bridge == NULL) {
        ret = EINVAL;
        goto out_err;
    }

    ret = iface_set_down(vif);
    if (ret) {
        goto out_err;
    }

    ret = bridge_rem_if(bridge, vif);
    if (ret) {
        goto out_err;
    }

    free(bridge);

    return 0;

out_err:
    if (bridge) {
        free(bridge);
    }

    return ret;
}

int vif_hotplug_online_noxs(const char* vif, const char* bridge)
{
    int ret;

    ret = bridge_add_if(bridge, vif);
    if (ret) {
        goto out_err;
    }

    ret = iface_set_up(vif);
    if (ret) {
        goto out_err;
    }

    return 0;

out_err:
    return ret;
}

int vif_hotplug_offline_noxs(const char* vif, const char* bridge)
{
    int ret;

    ret = iface_set_down(vif);
    if (ret) {
        goto out_err;
    }

    ret = bridge_rem_if(bridge, vif);
    if (ret) {
        goto out_err;
    }

    return 0;

out_err:
    return ret;
}
