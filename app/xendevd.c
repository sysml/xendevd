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

#include <fcntl.h>
#include <libudev.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

                default:
                    break;
            }
    }
}


int main(int argc, char** argv)
{
    int fd = -1;
    struct udev* udev = NULL;
    struct udev_monitor* mon = NULL;
    struct udev_device *dev = NULL;

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
    loop_ctrl_open(&loop_ctrl);

    /* setup udev */
    udev = udev_new();
    mon = udev_monitor_new_from_netlink(udev, "kernel");

    udev_monitor_filter_add_match_subsystem_devtype(mon, "xen-backend", NULL);
    udev_monitor_filter_add_match_subsystem_devtype(mon, "xen-backend-noxs", NULL);

    udev_monitor_enable_receiving(mon);

    fd = udev_monitor_get_fd(mon);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);


    /* setup xenstore */
    xs = xs_open(0);


    /*  main loop */
    while (1) {
        dev = udev_monitor_receive_device(mon);

        if (dev) {
            struct opinfo op;
            const char* action = udev_device_get_action(dev);
            const char* sysname = udev_device_get_sysname(dev);
            const char* subsystem = udev_device_get_subsystem(dev);

            if (strcmp(action, "add") == 0) {
                op.act = ACT_ADD;
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

            do_hotplug(&op, dev, xs, &loop_ctrl);

out:
            udev_device_unref(dev);
        }
    }

    /* FIXME: remove pid file upon exit*/
}
