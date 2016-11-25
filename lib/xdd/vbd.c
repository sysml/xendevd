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

#include <xdd/vbd.h>
#include <xdd/xs_helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


static int vbd_phy_hotplug_online_xs(struct xs_handle* xs, const char* xb_path, const char* device,
        char** err_str)
{
    int ret;
    char* dev_id;
    struct stat st;

    ret = stat(device, &st);
    if (ret) {
        ret = errno;
        if (errno == ENOENT) {
            asprintf(err_str, "%s does not exist.", device);
        } else {
            asprintf(err_str, "stat(%s) returned %d.", device, errno);
        }
        goto out_err;
    }

    /* FIXME: Check if dev is block device */
    if (!S_ISBLK(st.st_mode)) {
        ret = EINVAL;
        asprintf(err_str, "%s is not a block device.", device);
        goto out_err;
    }

    /* FIXME: Check device sharing */

    asprintf(&dev_id, "%x:%x", major(st.st_rdev), minor(st.st_rdev));
    xs_write_k(xs, dev_id, xb_path, "physical-device");
    free(dev_id);

    return 0;

out_err:
    return ret;
}

int vbd_hotplug_online_xs(struct xs_handle* xs, const char* xb_path)
{
    int ret = 0;
    char* type = NULL;
    char* device = NULL;
    char* err_str = NULL;

    type = xs_read_k(xs, xb_path, "type");
    if (type == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device type from xenstore");
        goto out_err;
    }

    device = xs_read_k(xs, xb_path, "params");
    if (device == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device params from xenstore");
        goto out_err;
    }

    if (strcmp(type, "phy") == 0) {
        ret = vbd_phy_hotplug_online_xs(xs, xb_path, device, &err_str);
    } else {
        ret = ENOSYS;
        asprintf(&err_str, "Device type %s for device %s is not supported.", type, device);
    }
    if (ret) {
        goto out_err;
    }

    free(type);
    free(device);

    return 0;

out_err:
    if (type) {
        free(type);
    }
    if (device) {
        free(device);
    }

    xs_write_k(xs, err_str, xb_path, "hotplug-error");
    xs_write_k(xs, "error", xb_path, "hotplug-status");
    free(err_str);

    return ret;
}
