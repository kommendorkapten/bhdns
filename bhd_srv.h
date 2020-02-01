/*
* Copyright (C) 2020 Fredrik Skogman, skogman - at - gmail.com.
*
* The contents of this file are subject to the terms of the Common
* Development and Distribution License (the "License"). You may not use this
* file except in compliance with the License. You can obtain a copy of the
* License at http://opensource.org/licenses/CDDL-1.0. See the License for the
* specific language governing permissions and limitations under the License.
* When distributing the software, include this License Header Notice in each
* file and include the License file at http://opensource.org/licenses/CDDL-1.0.
*/

#ifndef BHD_SRV_H
#define BHD_SRV_H

#include <netinet/in.h>

struct bhd_bl;
struct bhd_cfg;

/* Naming is based on responses, i.e upstream is where requests are
   forwarded */
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
        int fd_stats;
        char daemon;
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

#endif /* BHD_SRV_H */
