#include <xdd/loop.h>

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/loop.h>
#include <errno.h>


int loop_ctrl_open(struct xdd_loop_ctrl_handle* loop_if)
{
    int ret;

    ret = 0;

    if (loop_if == NULL) {
        ret = EINVAL;
        goto out_err;
    }

    loop_if->fd = open("/dev/loop-control", O_RDWR);
    if (loop_if->fd == -1) {
        ret = errno;
    }

out_err:
    return ret;
}

void loop_ctrl_close(struct xdd_loop_ctrl_handle* loop_if)
{
    if (loop_if) {
        close(loop_if->fd);
    }
}

int loop_ctrl_next_available_dev(struct xdd_loop_ctrl_handle* loop_if, char** out_dev)
{
    int ret;
    int devnum;

    ret = 0;

    if (loop_if == NULL || out_dev == NULL) {
        ret = EINVAL;
        goto out_err;
    }

    devnum = ioctl(loop_if->fd, LOOP_CTL_GET_FREE);
    if (devnum < 0) {
        ret = errno;
        goto out_err;
    }

    asprintf(out_dev, "/dev/loop%d", devnum);

out_err:
    return ret;
}

int loop_dev_bind(const char* device, const char* filename, const char* mode)
{
    int ret;
    int dev_fd, file_fd;
    mode_t file_mode;
    struct loop_info info;

    if (device == NULL || filename == NULL || mode == NULL) {
        ret = EINVAL;
        goto out_err;
    }

    if (strcmp(mode, "w") == 0) {
        file_mode = O_RDWR;
    } else if (strcmp(mode, "r") == 0) {
        file_mode = O_RDONLY;
    } else {
        ret = EINVAL;
        goto out_err;
    }

    dev_fd = open(device, O_RDWR);
    if (dev_fd < 0) {
        ret = errno;
        goto out_err;
    }

    file_fd = open(filename, file_mode);
    if (file_fd < 0) {
        ret = errno;
        goto out_close_device;
    }

    ret = ioctl(dev_fd, LOOP_SET_FD, file_fd);
    if (ret) {
        ret = errno;
        goto out_close_file;
    }

    ret = ioctl(dev_fd, LOOP_GET_STATUS, &info);
    if (ret) {
        ret = errno;
        goto out_close_file;
    }

    if (file_mode == O_RDONLY) {
        info.lo_flags |= LO_FLAGS_READ_ONLY;
    } else {
        info.lo_flags &= ~LO_FLAGS_READ_ONLY;
    }

    ret = ioctl(dev_fd, LOOP_SET_STATUS, &info);
    if (ret) {
        ret = errno;
        goto out_close_file;
    }

out_close_file:
    close(file_fd);
out_close_device:
    close(dev_fd);
out_err:
    return ret;
}

int loop_dev_unbind(const char* device)
{
    int ret;
    int dev_fd;

    if (device == NULL) {
        ret = EINVAL;
        goto out_err;
    }

    dev_fd = open(device, O_RDWR);
    if (dev_fd < 0) {
        ret = errno;
        goto out_err;
    }

    ret = ioctl(dev_fd, LOOP_CLR_FD, 0);
    if (ret) {
        ret = errno;
    }

    close(dev_fd);
out_err:
    return ret;
}

