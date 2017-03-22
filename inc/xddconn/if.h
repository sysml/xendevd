#ifndef __XDD__XDDCONN__IF__HH__
#define __XDD__XDDCONN__IF__HH__


#define XDD_SOCKET "/var/run/chaos_socket"

#define XDD_BLK_TYPE_SIZE    6
#define XDD_BLK_MODE_SIZE    2

struct xdd_blk_devreq {
    union {
        struct {
            char type[XDD_BLK_TYPE_SIZE];
            char mode[XDD_BLK_MODE_SIZE];
            char filename[0];
        } add;

        struct {
            char type[XDD_BLK_TYPE_SIZE];
            int major;
            int minor;
        } remove;

        struct {
            char type[XDD_BLK_TYPE_SIZE];
            int major;
            int minor;
        } query;
    };
};

struct xdd_blk_devrsp {
    union {
        struct {
            int major;
            int minor;
        } add;

        struct {

        } remove;

        struct {
            char filename[0];
        } query;
    };
};


enum xdd_devreq_type {
    dev_cmd_none,
    dev_cmd_add,
    dev_cmd_remove,
    dev_cmd_query
};

enum xdd_dev_type {
    dev_none,
    dev_blk,
};

struct xdd_devreq_hdr {
    enum xdd_devreq_type req_type;
    enum xdd_dev_type dev_type;
    int payload_len;
};

struct xdd_devreq {
    struct xdd_devreq_hdr hdr;

    union {
        struct xdd_blk_devreq* blk;
        void* payload;
    };
};

struct xdd_devrsp_hdr {
    int error;
    int payload_len;
};

struct xdd_devrsp {
    struct xdd_devrsp_hdr hdr;

    union {
        struct xdd_blk_devrsp* blk;
        void* payload;
    };
};


struct xddconn_server {
    int listenfd;
};

struct xddconn_client {
    int fd;
};

struct xddconn_session {
    struct xddconn_client client;
    struct xdd_devreq req;
    struct xdd_devrsp rsp;
};

int xddconn_server_open(struct xddconn_server* server);
int xddconn_server_recv_request(struct xddconn_server* conn,
        struct xddconn_session* session);
int xddconn_server_send_response(struct xddconn_server* conn,
        struct xddconn_session* session);

int xddconn_client_request(struct xdd_devreq* req, struct xdd_devrsp* rsp);

#endif /* __XDD__XDDCONN__IF__HH__ */
