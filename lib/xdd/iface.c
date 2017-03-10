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

#include <xdd/iface.h>

#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/sockios.h>


static int iface_flag_set(int flag, const char* dev)
{
    int err;
    struct ifreq ifr;

    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		return errno;
    }

    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    err = ioctl(sock_fd, SIOCGIFFLAGS, &ifr);
    if (err < 0) {
        return errno;
    }

    if (flag < 0) {
        flag = -flag;
        ifr.ifr_flags &= ~flag;
    } else {
        ifr.ifr_flags |= flag;
    }

    err = ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
    if (err < 0) {
        return errno;
    }

    err = close(sock_fd);
    if (err < 0) {
        return errno;
    }

    return 0;
}

int iface_set_up(const char* dev)
{
    return iface_flag_set(IFF_UP, dev);
}

int iface_set_down(const char* dev)
{
    return iface_flag_set(-IFF_UP, dev);
}
