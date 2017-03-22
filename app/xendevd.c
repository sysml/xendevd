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

#include <xdd/loop.h>
#include <xdd/vbd.h>
#include <xdd/vif.h>
#include <xddconn/if.h>

#include <fcntl.h>
#include <libudev.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <xenstore.h>


enum action {
    ACT_ADD ,
    ACT_REMOVE ,
    ACT_ONLINE ,
    ACT_OFFLINE ,
};

enum backend {
    BE_XS ,
    BE_NOXS ,
};

enum device_type {
    DEV_T_VIF ,
    DEV_T_VBD ,
};

struct opinfo {
    enum action act;
    enum backend be;

    enum device_type dev_t;
    union {
        struct {
            const char* xb_path;
            const char* dev_name;
            const char* bridge;
        } vif;

        struct {
            const char* xb_path;
        } vbd;
    } dev_info;
};

struct xdd_conf {
    bool help;
    bool daemonize;
    bool write_pid_file;
    char* pid_file;
};

static void init_xdd_conf(struct xdd_conf* conf)
{
    conf->help = false;
    conf->daemonize = false;
    conf->write_pid_file = false;
    conf->pid_file = "/var/run/xendevd.pid";
}

static int parse_args(int argc, char** argv, struct xdd_conf* conf)
{
    const char *short_opts = "hD";
    const struct option long_opts[] = {
        { "help"               , no_argument       , NULL , 'h' },
        { "daemon"             , no_argument       , NULL , 'D' },
        { "pid-file"           , required_argument , NULL , 'p' },
        { NULL , 0 , NULL , 0 }
    };

    int opt;
    int opt_index;
    int error = 0;

    while (1) {
        opt = getopt_long(argc, argv, short_opts, long_opts, &opt_index);

        if (opt == -1) {
            break;
        }

        switch (opt) {
            case 'h':
                conf->help = true;
                break;

            case 'D':
                conf->daemonize = true;
                conf->write_pid_file = true;
                break;

            case 'p':
                conf->write_pid_file = true;
                conf->pid_file = optarg;
                break;

            default:
                error = true;
                break;
        }
    }

    while (optind < argc) {
        error = 1;

        printf("%s: invalid argument \'%s\'\n", argv[0], argv[optind]);
        optind++;
    }

    return error;
}

static void print_usage(char* cmd)
{
    printf("Usage: %s [OPTION]...\n", cmd);
    printf("\n");
    printf("Options:\n");
    printf("  -D, --daemon           Run in background\n");
    printf("  -h, --help             Display this help and exit\n");
    printf("      --pid-file <file>  Write process pid to file [default: /var/run/xendevd.pid]\n");
}

static void do_hotplug(struct opinfo* op, struct udev_device* dev, struct xs_handle* xs, struct xdd_loop_ctrl_handle* loop_ctrl)
{
    switch (op->be) {
        case BE_XS:
            switch (op->dev_t) {
                case DEV_T_VIF: {
                    const char* xb_path = udev_device_get_property_value(dev, "XENBUS_PATH");
                    const char* vif = udev_device_get_property_value(dev, "vif");

                    switch (op->act) {
                        case ACT_ONLINE:
                            vif_hotplug_online_xs(xs, xb_path, vif);
                            break;

                        case ACT_OFFLINE:
                            vif_hotplug_offline_xs(xs, xb_path, vif);
                            break;

                        default:
                            break;
                    }
                } break;

                case DEV_T_VBD: {
                    const char* xb_path = udev_device_get_property_value(dev, "XENBUS_PATH");

                    switch (op->act) {
                        case ACT_ADD:
                            vbd_hotplug_online_xs(loop_ctrl, xs, xb_path);
                            break;

                        case ACT_REMOVE:
                            vbd_hotplug_offline_xs(xs, xb_path);
                            break;

                        default:
                            break;
                    }
                } break;
            }
            break;

        case BE_NOXS:
            switch (op->dev_t) {
                case DEV_T_VIF: {
                    const char* vif = udev_device_get_property_value(dev, "vif");
                    const char* bridge = udev_device_get_property_value(dev, "bridge");

                    switch (op->act) {
                        case ACT_ONLINE:
                            vif_hotplug_online_noxs(vif, bridge);
                            break;

                        case ACT_OFFLINE:
                            vif_hotplug_offline_noxs(vif, bridge);
                            break;

                        default:
                            break;
                    }
                } break;

                case DEV_T_VBD: {
                    switch (op->act) {
                        case ACT_ADD:
                            break;

                        case ACT_REMOVE:
                            break;

                        default:
                            break;
                    }
                } break;

                default:
                    break;
            }
    }
}


struct udev_if {
    struct udev_monitor* mon;
    int fd;
};

static int udev_if_init(struct udev_if* udev_if)
{
    int ret;
    int fd = -1;
    struct udev* udev = NULL;
    struct udev_monitor* mon = NULL;

    ret = 0;

    udev = udev_new();
    if (udev == NULL) {
        ret = errno;
        goto out_err;
    }

    mon = udev_monitor_new_from_netlink(udev, "kernel");
    if (mon == NULL) {
        ret = errno;
        goto out_err;
    }

    ret = udev_monitor_filter_add_match_subsystem_devtype(mon, "xen-backend", NULL);
    if (ret) {
        ret = errno;
        goto out_err;
    }

    ret = udev_monitor_filter_add_match_subsystem_devtype(mon, "xen-backend-noxs", NULL);
    if (ret) {
        ret = errno;
        goto out_err;
    }

    ret = udev_monitor_enable_receiving(mon);
    if (ret) {
        ret = errno;
        goto out_err;
    }

    fd = udev_monitor_get_fd(mon);
    if (fd < 0) {
        ret = errno;
        goto out_err;
    }

    ret = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
    if (ret) {
        ret = errno;
        goto out_err;
    }

    udev_if->mon = mon;
    udev_if->fd = fd;

    return 0;

out_err:
    if (udev) {
        udev_unref(udev);
    }
    if (mon) {
        udev_monitor_unref(mon);
    }
    return ret;
}

static void udev_if_event(struct udev_if* udev_if, struct xs_handle* xs,
        struct xdd_loop_ctrl_handle* loop_ctrl)
{
    struct udev_device* dev;

    dev = udev_monitor_receive_device(udev_if->mon);

    if (dev) {
        struct opinfo op;
        const char* action = udev_device_get_action(dev);
        const char* sysname = udev_device_get_sysname(dev);
        const char* subsystem = udev_device_get_subsystem(dev);

        if (strcmp(action, "add") == 0) {
            op.act = ACT_ADD;
        } else if (strcmp(action, "remove") == 0) {
            op.act = ACT_REMOVE;
        } else if (strcmp(action, "online") == 0) {
            op.act = ACT_ONLINE;
        } else if (strcmp(action, "offline") == 0) {
            op.act = ACT_OFFLINE;
        } else {
            goto out;
        }

        if (strcmp(subsystem, "xen-backend") == 0) {
            op.be = BE_XS;
        } else if (strcmp(subsystem, "xen-backend-noxs") == 0) {
            op.be = BE_NOXS;
        } else {
            goto out;
        }

        if (strncmp(sysname, "vif-", 4) == 0) {
            op.dev_t = DEV_T_VIF;
        } else if (strncmp(sysname, "vbd", 3) == 0) {
            op.dev_t = DEV_T_VBD;
        } else {
            goto out;
        }

        do_hotplug(&op, dev, xs, loop_ctrl);

out:
        udev_device_unref(dev);
    }
}

static int udev_if_get_device_name(struct udev_if* udev_if,
        int major, int minor, char** out_devname)
{
    int ret;
    struct udev *udev;
    struct udev_device *device;
    const char* devname;

    ret = 0;

    udev = udev_monitor_get_udev(udev_if->mon);
    if (udev == NULL) {
        ret = errno;
        goto out_ret;
    }

    device = udev_device_new_from_devnum(udev, 'b', makedev(major, minor));
    if (device == NULL) {
        ret = errno;
        goto out_ret;
    }

    devname = udev_device_get_property_value(device, "DEVNAME");
    if (devname == NULL) {
        ret = errno;
        goto out_unref;
    }

    *out_devname = strdup(devname);

out_unref:
    udev_device_unref(device);
out_ret:
    return ret;
}

static int xdd_blk_op(struct xdd_devreq* req, struct xdd_devrsp* rsp,
        struct udev_if* udev_if, struct xdd_loop_ctrl_handle* loop_ctrl)
{
    int ret;
    struct xdd_blk_devreq* blk_req = req->blk;
    struct xdd_blk_devrsp* blk_rsp = NULL;
    char* devname = NULL;
    int payload_len = 0;

    if (req->hdr.req_type == dev_cmd_add) {
        payload_len = sizeof(blk_rsp->add);

        blk_rsp = malloc(payload_len);
        if (blk_rsp == NULL) {
            payload_len = 0;
            ret = errno;
            goto out;
        }

        ret = vbd_hotplug_online_noxs(loop_ctrl,
                blk_req->add.filename, blk_req->add.type, blk_req->add.mode,
                &blk_rsp->add.major, &blk_rsp->add.minor);

        if (ret) {
            free(blk_rsp);
            blk_rsp = NULL;
            payload_len = 0;
        }

    } else if (req->hdr.req_type == dev_cmd_remove) {
        ret = udev_if_get_device_name(udev_if,
                blk_req->remove.major, blk_req->remove.minor, &devname);

        if (ret == 0) {
            ret = vbd_hotplug_offline_noxs(devname, blk_req->remove.type);
        }

    } else if (req->hdr.req_type == dev_cmd_query) {
        char* backed = NULL;
        const char* filename;

        ret = udev_if_get_device_name(udev_if,
                blk_req->query.major, blk_req->query.minor, &devname);
        if (ret) {
            goto out;
        }

        ret = vbd_backed_file(devname, blk_req->query.type, &backed);
        if (ret) {
            goto out_free_query;
        }

        if (backed) {
            filename = backed;
        } else {
            filename = devname;
        }

        payload_len = sizeof(blk_rsp->query) + strlen(filename) + 1;

        blk_rsp = malloc(payload_len);
        if (blk_rsp == NULL) {
            payload_len = 0;
            ret = errno;
            goto out_free_query;
        }

        strcpy(blk_rsp->query.filename, filename);

out_free_query:
        if (backed) {
            free(backed);
        }

    } else {
        ret = EINVAL;
        goto out;
    }

out:
    rsp->hdr.error = ret;
    rsp->hdr.payload_len = payload_len;
    rsp->payload = blk_rsp;

    if (devname) {
        free(devname);
    }

    return ret;
}

static int xddconn_message(struct xddconn_server* xddconn,
        struct udev_if* udev_if, struct xdd_loop_ctrl_handle* loop_ctrl)
{
    int ret;
    struct xddconn_session session;

    ret = xddconn_server_recv_request(xddconn, &session);
    if (ret) {
        ret = errno;
        goto out_ret;
    }

    if (session.req.hdr.dev_type == dev_blk) {
        xdd_blk_op(&session.req, &session.rsp, udev_if, loop_ctrl);

    } else {
        session.rsp.hdr.error = EINVAL;
        session.rsp.hdr.payload_len = 0;
    }

    ret = xddconn_server_send_response(xddconn, &session);
    if (ret) {
        ret = errno;
        goto out_ret;
    }

out_ret:
    return ret;
}


int main(int argc, char** argv)
{
    struct udev_if udev_if;
    struct xddconn_server xddconn;
    struct xs_handle *xs = NULL;

    int err;
    struct xdd_conf conf;
    struct xdd_loop_ctrl_handle loop_ctrl;

    FILE* pidf = NULL;


    /* Parse arguments */
    init_xdd_conf(&conf);

    err = parse_args(argc, argv, &conf);
    if (err || conf.help) {
        print_usage(argv[0]);
        return err ? -EINVAL : 0;
    }

    if (conf.write_pid_file) {
        pidf = fopen(conf.pid_file, "w");
    }

    if (conf.daemonize) {
        err = daemon(0, 0);
        if (err) {
            printf("Cannot daemonize.");
            return err;
        }
    }

    if (conf.write_pid_file) {
        fprintf(pidf, "%d", getpid());
        fclose(pidf);
    }

    /* setup interface to loop device control */
    memset(&loop_ctrl, 0, sizeof(loop_ctrl));
    err = loop_ctrl_open(&loop_ctrl);
    if (err) {
        fprintf(stderr, "Error opening loop device control.");
        err = errno;
        goto out_err;
    }

    /* setup udev */
    memset(&udev_if, 0, sizeof(udev_if));
    err = udev_if_init(&udev_if);
    if (err) {
        fprintf(stderr, "Error initializing udev interface.");
        err = errno;
        goto out_err;
    }

    /* setup toolstack connection */
    memset(&xddconn, 0, sizeof(xddconn));
    err = xddconn_server_open(&xddconn);
    if (err) {
        fprintf(stderr, "Error opening xdd connection.");
        err = errno;
        goto out_err;
    }

    /* setup xenstore */
    xs = xs_open(0);


    /*  main loop */
    while (1) {
        struct pollfd fds[2];

        fds[0].fd = udev_if.fd;
        fds[1].fd = xddconn.listenfd;
        fds[0].events = POLLIN | POLLPRI;
        fds[1].events = POLLIN | POLLPRI;

        err = poll(fds, 2, -1);

        if (fds[0].revents & POLLIN) {
            udev_if_event(&udev_if, xs, &loop_ctrl);
        }

        if (fds[1].revents & POLLIN) {
            xddconn_message(&xddconn, &udev_if, &loop_ctrl);
        }
    }

    /* FIXME: remove pid file upon exit*/

    /* FIXME: free udev if, xddconn and xs resources upon exit */

out_err:
    return err;
}
