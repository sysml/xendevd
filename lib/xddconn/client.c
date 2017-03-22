#include <xddconn/if.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


int xddconn_client_request(struct xdd_devreq* req, struct xdd_devrsp* rsp)
{
    int ret;
    int sockfd, addrlen;
    struct sockaddr_un addr;
    void* payload = NULL;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ret = errno;
        goto out_ret;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, XDD_SOCKET);
    addrlen = strlen(addr.sun_path) + sizeof(addr.sun_family);

    ret = connect(sockfd, (struct sockaddr*) &addr, addrlen);
    if (ret) {
        ret = errno;
        goto out_close;
    }

    ret = send(sockfd, &req->hdr, sizeof(req->hdr), 0);
    if (ret != sizeof(req->hdr)) {
        ret = errno;
        goto out_ret;
    }

    if (req->hdr.payload_len > 0) {
        ret = send(sockfd, req->payload, req->hdr.payload_len, 0);
        if (ret != req->hdr.payload_len) {
            ret = errno;
            goto out_close;
        }
    }

    ret = recv(sockfd, &rsp->hdr, sizeof(rsp->hdr), 0);
    if (ret != sizeof(rsp->hdr)) {
        ret = errno;
        goto out_close;
    }

    if (rsp->hdr.payload_len > 0) {
        payload = malloc(rsp->hdr.payload_len);
        if (payload == NULL) {
            perror("Error allocating memory");
            ret = errno;
            goto out_close;
        }

        ret = recv(sockfd, payload, rsp->hdr.payload_len, 0);
        if (ret != rsp->hdr.payload_len) {
            free(payload);
            ret = errno;
            goto out_close;
        }
    }

    rsp->payload = payload;

    ret = 0;

out_close:
    close(sockfd);
out_ret:
    return ret;
}
