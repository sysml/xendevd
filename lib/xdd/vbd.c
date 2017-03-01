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


#define TYPE_PHYSICAL  "phy"
#define TYPE_FILE      "file"

enum file_type {
    file_none,
    file_regular,
    file_block
};
typedef enum file_type file_type_t;


static int check_file_type(const char* filename, file_type_t check_type,
        struct stat *out_st, char** err_str)
{
    int ret;
    struct stat st;

    ret = stat(filename, &st);
    if (ret) {
        ret = errno;
        if (errno == ENOENT) {
            asprintf(err_str, "%s does not exist.", filename);
        } else {
            asprintf(err_str, "stat(%s) returned %d.", filename, errno);
        }
        goto out_err;
    }

    if (check_type == file_regular && !S_ISREG(st.st_mode)) {
        ret = EINVAL;
        asprintf(err_str, "%s is not a regular file.", filename);
        goto out_err;

    } else if (check_type == file_block && !S_ISBLK(st.st_mode)) {
        ret = EINVAL;
        asprintf(err_str, "%s is not a block device.", filename);
        goto out_err;
    }

    if (out_st)
        *out_st = st;

    return 0;

out_err:
    return ret;
}

static int vbd_phy_hotplug_online_xs(struct xs_handle* xs, const char* xb_path,
        const char* device, char** err_str)
{
    int ret;
    char* dev_id;
    struct stat st;

    ret = check_file_type(device, file_block, &st, err_str);
    if (ret) {
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

static int vbd_file_hotplug_online_xs(struct xdd_loop_ctrl_handle* loop_ctrl,
        struct xs_handle* xs, const char* xb_path,
        const char* filename, const char* mode,
        char** err_str)
{
    int ret;
    char* device = NULL;

    ret = check_file_type(filename, file_regular, NULL, err_str);
    if (ret) {
        goto out_err;
    }

    ret = loop_ctrl_next_available_dev(loop_ctrl, &device);
    if (ret) {
        asprintf(err_str, "Error finding next loop device for %s (%s).",
                filename, strerror(ret));
        goto out_err;
    }

    ret = loop_dev_bind(device, filename, mode);
    if (ret) {
        asprintf(err_str, "Error binding to loop device for %s (%s).",
                filename, strerror(ret));
        goto out_device;
    }

    xs_write_k(xs, device, xb_path, "node");

    ret = vbd_phy_hotplug_online_xs(xs, xb_path, device, err_str);
    if (ret) {
        goto out_device;
    }

out_device:
    free(device);
out_err:
    return ret;
}

static int vbd_file_hotplug_offline_xs(struct xs_handle* xs, const char* xb_path,
        char** err_str)
{
    int ret;
    char* device = NULL;

    device = xs_read_k(xs, xb_path, "node");
    if (device == NULL) {
        ret = EINVAL;
        asprintf(err_str, "Can't read device node from xenstore");
        goto out_err;
    }

    ret = check_file_type(device, file_block, NULL, err_str);
    if (ret) {
        goto out_err;
    }

    ret = loop_dev_unbind(device);
    if (ret) {
        asprintf(err_str, "Error unbinding loop device %s (%s).",
                device, strerror(ret));
        goto out_device;
    }

out_device:
    free(device);
out_err:
    return ret;
}

int vbd_hotplug_online_xs(struct xdd_loop_ctrl_handle* loop_ctrl,
        struct xs_handle* xs, const char* xb_path)
{
    int ret = 0;
    char* toolstack = NULL;
    char* type = NULL;
    char* params = NULL;
    char* mode = NULL;
    char* err_str = NULL;

    /* We only add devices created with chaos */
    toolstack = xs_read_k(xs, xb_path, "toolstack");
    if (toolstack == NULL || strcmp(toolstack, "chaos") != 0) {
        goto out_ret;
    }

    params = xs_read_k(xs, xb_path, "params");
    if (params == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device params from xenstore");
        goto out_err;
    }

    type = xs_read_k(xs, xb_path, "type");
    if (type == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device type from xenstore");
        goto out_err;
    }

    mode = xs_read_k(xs, xb_path, "mode");
    if (mode == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device mode from xenstore");
        goto out_err;
    }

    if (strcmp(type, TYPE_PHYSICAL) == 0) {
        ret = vbd_phy_hotplug_online_xs(xs, xb_path, params, &err_str);
    } else if (strcmp(type, TYPE_FILE) == 0) {
        ret = vbd_file_hotplug_online_xs(loop_ctrl, xs, xb_path, params, mode, &err_str);
    } else {
        ret = ENOSYS;
        asprintf(&err_str, "Device type %s for device %s is not supported.", type, params);
    }
    if (ret) {
        goto out_err;
    }

    free(toolstack);
    free(type);
    free(params);

    return 0;

out_err:
    if (type) {
        free(type);
    }
    if (params) {
        free(params);
    }

    xs_write_k(xs, err_str, xb_path, "hotplug-error");
    xs_write_k(xs, "error", xb_path, "hotplug-status");
    free(err_str);

out_ret:
    if (toolstack) {
        free(toolstack);
    }

    return ret;
}

int vbd_hotplug_offline_xs(struct xs_handle* xs, const char* xb_path)
{
    int ret = 0;
    char* toolstack = NULL;
    char* type = NULL;
    char* params = NULL;
    char* err_str = NULL;

    /* We only add devices created with chaos */
    toolstack = xs_read_k(xs, xb_path, "toolstack");
    if (toolstack == NULL || strcmp(toolstack, "chaos") != 0) {
        goto out_ret;
    }

    params = xs_read_k(xs, xb_path, "params");
    if (params == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device params from xenstore");
        goto out_err;
    }

    type = xs_read_k(xs, xb_path, "type");
    if (type == NULL) {
        ret = EINVAL;
        err_str = strdup("Can't read device type from xenstore");
        goto out_err;
    }

    if (strcmp(type, TYPE_PHYSICAL) == 0) {
        /* Nothing */
    } else if (strcmp(type, TYPE_FILE) == 0) {
        ret = vbd_file_hotplug_offline_xs(xs, xb_path, &err_str);
    } else {
        ret = ENOSYS;
        asprintf(&err_str, "Device type %s for device %s is not supported.", type, params);
    }
    if (ret) {
        goto out_err;
    }

    free(toolstack);
    free(type);
    free(params);

    return 0;

out_err:
    if (type) {
        free(type);
    }
    if (params) {
        free(params);
    }

    xs_write_k(xs, err_str, xb_path, "hotplug-error");
    xs_write_k(xs, "error", xb_path, "hotplug-status");
    free(err_str);

out_ret:
    if (toolstack) {
        free(toolstack);
    }

    return ret;
}
