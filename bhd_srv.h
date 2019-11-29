#ifndef BHD_SRV_H
#define BHD_SRV_H

#include <netinet/in.h>

struct bhd_bl;
struct bhd_cfg;

struct bhd_stats
{
        size_t numf;
        size_t numb;
        size_t up_tx;
        size_t up_rx;
        size_t down_tx;
        size_t down_rx;
};

struct bhd_srv
{
        struct bhd_stats stats;
        struct sockaddr_in faddr;
        const struct bhd_cfg* cfg;
        struct bhd_bl* bl;
        int fd_listen;
        int fd_forward;
        int daemon;
};

/**
 * Initilize a bhd_srv struct.
 * @param srv the struct to initialize.
 * @param cfg configuration struct.
 * @param bl block list to use.
 * @param daemon set to true if srv should be run as a daemon.
 * @return 0 on success.
 */
int bhd_srv_init(struct bhd_srv* srv,
                 const struct bhd_cfg* cfg,
                 struct bhd_bl* bl,
                 int daemon);

/**
 * Start server.
 * @param srv struct to use
 * @return -1 if server unexpectedly stopped.
 */
int bhd_serve(struct bhd_srv* srv);

#endif /* __BHD_SRV_H__ */
