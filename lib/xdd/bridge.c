#include <xdd/bridge.h>

#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/sockios.h>


static int bridge_if(int op, const char* bridge, const char* dev)
{
	int err;
	struct ifreq ifr;

	int ifindex = if_nametoindex(dev);
	if (ifindex == 0) {
		return ENODEV;
    }

    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		return errno;
    }

	strncpy(ifr.ifr_name, bridge, IFNAMSIZ);
	ifr.ifr_ifindex = ifindex;

	err = ioctl(sock_fd, op, &ifr);
    if (err < 0) {
        return errno;
    }

    err = close(sock_fd);
    if (err < 0) {
        return errno;
    }

    return 0;
}

int bridge_add_if(const char* bridge, const char* dev)
{
    return bridge_if(SIOCBRADDIF, bridge, dev);
}

int bridge_rem_if(const char* bridge, const char* dev)
{
    return bridge_if(SIOCBRDELIF, bridge, dev);
}

