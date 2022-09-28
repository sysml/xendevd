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
 */

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
