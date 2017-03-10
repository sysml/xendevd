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
    xs = xs_open(0);
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
