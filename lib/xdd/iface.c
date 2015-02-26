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
