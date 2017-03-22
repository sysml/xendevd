#include <xddconn/if.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


int xddconn_server_open(struct xddconn_server* server)
{
    int ret;
    int sockfd, addrlen;
    struct sockaddr_un addr;

    if (server == NULL) {
        ret = EINVAL;
        goto out_ret;
    }

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ret = errno;
        perror("Error opening connection socket");
        goto out_ret;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, XDD_SOCKET);
    unlink(addr.sun_path);

    addrlen = strlen(addr.sun_path) + sizeof(addr.sun_family);
    ret = bind(sockfd, (struct sockaddr *) &addr, addrlen);
    if (ret) {
        perror("Error binding toolstack connection socket");
        ret = errno;
        goto out_close;
    }

    ret = listen(sockfd, 1);
    if (ret) {
        perror("Error starting connection listening");
        ret = errno;
        goto out_close;
    }

    server->listenfd = sockfd;

    return 0;

out_close:
    close(sockfd);
out_ret:
    return ret;
}

int xddconn_server_recv_request(struct xddconn_server* server,
        struct xddconn_session* session)
{
    int ret;
    int sockfd;
    struct sockaddr_un addr;
    socklen_t addrlen;
    void* payload = NULL;

    if (server == NULL || session == NULL) {
        ret = EINVAL;
        goto out_ret;
    }

    addrlen = sizeof(addr);

    sockfd = accept(server->listenfd, (struct sockaddr*) &addr, &addrlen);
    if (sockfd < 0) {
        perror("Error accepting connection");
        ret = errno;
        goto out_ret;
    }

    ret = recv(sockfd, &session->req.hdr, sizeof(session->req.hdr), 0);
    if (ret != sizeof(session->req.hdr)) {
        perror("Error receiving request header");
        ret = errno;
        goto out_close;
    }

    if (session->req.hdr.payload_len > 0) {
        payload = malloc(session->req.hdr.payload_len);
        if (payload == NULL) {
            perror("Error allocating memory");
            ret = errno;
            goto out_close;
        }

        ret = recv(sockfd, payload, session->req.hdr.payload_len, 0);
        if (ret != session->req.hdr.payload_len) {
            perror("Error receiving request payload");
            free(payload);
            ret = errno;
            goto out_close;
        }
    }

    session->client.fd = sockfd;
    session->req.payload = payload;

    return 0;

out_close:
    close(sockfd);
out_ret:
    return ret;
}


int xddconn_server_send_response(struct xddconn_server* server,
        struct xddconn_session* session)
{
    int ret;

    if (server == NULL || session == NULL) {
        ret = EINVAL;
        goto out_ret;
    }

    ret = send(session->client.fd, &session->rsp.hdr, sizeof(session->rsp.hdr), 0);
    if (ret != sizeof(session->rsp.hdr)) {
        perror("Error sending response header");
        ret = errno;
        goto out_close;
    }

    if (session->rsp.hdr.payload_len) {
        ret = send(session->client.fd, session->rsp.payload, session->rsp.hdr.payload_len, 0);
        if (ret != session->rsp.hdr.payload_len) {
            perror("Error sending response payload");
            ret = errno;
            goto out_close;
        }
    }

    ret = 0;

out_close:
    close(session->client.fd);

    if (session->req.payload) {
        free(session->req.payload);
        session->req.payload = NULL;
    }

    if (session->rsp.payload) {
        free(session->rsp.payload);
        session->rsp.payload = NULL;
    }
out_ret:
    return ret;
}
