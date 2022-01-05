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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <poll.h>
#include <ctype.h>
#include "bhd_srv.h"
#include "bhd_dns.h"
#include "bhd_bl.h"
#include "bhd_cfg.h"
#include "vendor/timing.h"

/* Max UDP message size from RFC1035 */
#define BUF_LEN 512
/* Default timeout in ms */
#define BHD_TIMEOUT 5000
sig_atomic_t run;

int bhd_srv_stat_str(char* buf, int len, const struct bhd_stats* stats);

/**
 * Return 0 if successful.
 */
static int bhd_srv_serve_dns(struct bhd_srv* srv);
static int bhd_srv_serve_stats(struct bhd_srv* srv);

/**
 * Default signal handler.
 */
static void sigh(int);

int bhd_srv_init(struct bhd_srv* srv,
                 const struct bhd_cfg* cfg,
                 struct bhd_bl* bl,
                 int daemon)
{
        struct sockaddr_in saddr;
        struct sigaction sa;
        int ret;

        srv->stats = (struct bhd_stats){.numf = 0,
                                        .numb = 0,
                                        .up_tx = 0,
                                        .up_rx = 0,
                                        .down_tx = 0,
                                        .down_rx = 0};
        srv->cfg = cfg;
        srv->bl = bl;
        srv->daemon = (char)daemon;

        run = 0;

        sa.sa_flags = 0;
        sa.sa_handler = &sigh;
        sigfillset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL))
        {
                syslog(LOG_WARNING, "Failed to install sighandler %m");
        }

        /* Set up forward address */
        syslog(LOG_INFO, "Listen address: %s@%d", cfg->laddr, cfg->lport);
        syslog(LOG_INFO, "Forward address: %s@%d", cfg->faddr, cfg->fport);
        srv->fd_forward = socket(AF_INET, SOCK_DGRAM, 0);
        if (srv->fd_forward < 0)
        {
                syslog(LOG_ERR, "Could not create socket: %m");
                return -1;
        }
        memset(&srv->faddr, 0, sizeof(srv->faddr));
        srv->faddr.sin_family = AF_INET;
        srv->faddr.sin_port = htons(cfg->fport);
        if (inet_pton(AF_INET, cfg->faddr, &srv->faddr.sin_addr) < 0)
        {
                syslog(LOG_ERR, "Invalid address '%s': %m", cfg->faddr);
                return -1;
        }

        /* Set up listening socket */
        srv->fd_listen = socket(AF_INET, SOCK_DGRAM, 0);
        if (srv->fd_listen < 0)
        {
                syslog(LOG_ERR, "Could not create socket: %m");
                return -1;
        }
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(cfg->lport);
        if (strncmp(cfg->laddr, "0.0.0.0", 7) == 0)
        {
                saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
                uint32_t na;

                if (inet_pton(AF_INET, cfg->laddr, &na) < 0)
                {
                        syslog(LOG_ERR, "Invalid address '%s': %m", cfg->laddr);
                        return -1;
                }

                saddr.sin_addr.s_addr = na;
        }
        ret = bind(srv->fd_listen, (struct sockaddr*)&saddr, sizeof(saddr));
        if (ret < 0)
        {
                syslog(LOG_ERR, "Failed to bind: %m");
                return -1;
        }

        /* Stats socket */
        srv->fd_stats = socket(AF_INET, SOCK_DGRAM, 0);
        if (srv->fd_stats < 0)
        {
                syslog(LOG_ERR, "Could not create socket: %m");
                return -1;
        }
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(cfg->sport);
        if (strncmp(cfg->laddr, "0.0.0.0", 7) == 0)
        {
                saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
                uint32_t na;

                if (inet_pton(AF_INET, cfg->laddr, &na) < 0)
                {
                        syslog(LOG_ERR, "Invalid address '%s': %m", cfg->laddr);
                        return -1;
                }

                saddr.sin_addr.s_addr = na;
        }
        ret = bind(srv->fd_stats, (struct sockaddr*)&saddr, sizeof(saddr));
        if (ret < 0)
        {
                syslog(LOG_ERR, "Failed to bind: %m");
                return -1;
        }

        return 0;
}

int bhd_serve(struct bhd_srv* srv)
{
        struct pollfd fds[2];
        int ret;

        fds[0].fd = srv->fd_listen;
        fds[0].events = POLLIN;
        fds[1].fd = srv->fd_stats;
        fds[1].events = POLLIN;

        run = 1;
        while(run)
        {
                int ready = poll(fds, 2, -1);

                if (ready < 0)
                {
                        /* error */
                        syslog(LOG_WARNING, "%s:poll:%m", __func__);
                        continue;
                }

                if (fds[0].revents & POLLIN)
                {
                        ret = bhd_srv_serve_dns(srv);
                        if (ret)
                        {
                                syslog(LOG_WARNING, "DNS query failed");
                        }
                }
                if (fds[1].revents & POLLIN)
                {
                        bhd_srv_serve_stats(srv);
                }
        }

        if (!srv->daemon)
        {
                printf("Stop listening\n");
                printf("Forwarded %ld requests\n", srv->stats.numf);
                printf("Blocked %ld requests\n", srv->stats.numb);
                printf("Upstream tx %ld bytes\n", srv->stats.up_tx);
                printf("Upstream rx %ld bytes\n", srv->stats.up_rx);
                printf("Downstream tx %ld bytes\n", srv->stats.down_tx);
                printf("Dowmstream rx %ld bytes\n", srv->stats.down_rx);
        }

        close(srv->fd_listen);
        close(srv->fd_forward);

        return 0;
}

static int bhd_srv_serve_dns(struct bhd_srv* srv)
{
        unsigned char buf[BUF_LEN];
        struct sockaddr_in caddr;
        struct bhd_dns_h h;
        struct bhd_dns_q_section qs;
        struct bhd_dns_rr rr;
        struct pollfd fds[1];
        size_t br;
        size_t offset = 0;
        ssize_t nb;
        socklen_t slen = sizeof(caddr);
        int ready;

        nb = recvfrom(srv->fd_listen,
                      buf,
                      BUF_LEN,
                      0,
                      (struct sockaddr*)&caddr,
                      &slen);
        if (nb < 0)
        {
                syslog(LOG_WARNING, "client:recvfrom: %m");
                return -1;
        }

        srv->stats.down_rx += nb;
        if (nb < BHD_DNS_H_SIZE)
        {
                syslog(LOG_WARNING,
                       "client:recvfrom %ld bytes, expected %d",
                       nb,
                       BHD_DNS_H_SIZE);
                return -1;
        }

        br = bhd_dns_h_unpack(&h, buf);
        qs.qd_count = h.qd_count;
        offset += br;
        br = bhd_dns_q_section_unpack(&qs, buf + offset);
        offset += br;

        for (uint16_t i = 0; i < h.an_count; i++)
        {
                br = bhd_dns_rr_unpack(&rr, buf + offset);
                offset += br;
        }
        for (uint16_t i = 0; i < h.ns_count; i++)
        {
                br = bhd_dns_rr_unpack(&rr, buf + offset);
                offset += br;
        }
        for (uint16_t i = 0; i < h.ar_count; i++)
        {
                br = bhd_dns_rr_unpack(&rr, buf + offset);
                offset += br;
        }

#if DEBUG
        bhd_dns_h_dump(&h);
#endif
        if (offset != (size_t)nb)
        {
#if DEBUG
                for (ssize_t i = (ssize_t)offset; i < nb; i++)
                {
                        printf("%02x:", buf[i]);
                }
                printf("\n");
#endif
                syslog(LOG_WARNING,
                       "Not all data was unpacked: got %ld want %ld",
                       offset,
                       nb);
                return -1;
        }

        if (h.qr == 0 &&
            h.opcode == BHD_DNS_OP_QUERY &&
            qs.qd_count == 1 &&
            qs.q->qtype == BHD_DNS_QTYPE_A &&
            qs.q->qclass == BHD_DNS_CLASS_IN)
        {
                struct bhd_dns_q_label* l = &qs.q->qname;
                int m = bhd_bl_match(srv->bl, l);

                if (m)
                {
                        /* Send static response */
                        struct bhd_dns_rr_a rr;

                        bhd_dns_rr_a_init(&rr, srv->cfg->baddr);
                        h.qr = 1;
                        h.ra = 1;
                        h.an_count = 1;

                        nb = bhd_dns_h_pack(buf, BUF_LEN, &h);
                        nb += bhd_dns_q_section_pack(buf + nb,
                                                     BUF_LEN - nb,
                                                     &qs);
                        nb += bhd_dns_rr_a_pack(buf + nb,
                                                BUF_LEN - nb,
                                                &rr);

                        srv->stats.numb++;
                        goto do_respond;
                }
        }

        srv->stats.numf++;

        /* Blocking of sendto(2) operations on an UDP socket is very unlikely
           to happen, so currently we omit calling poll(2). */
        nb = sendto(srv->fd_forward,
                    buf,
                    nb,
                    0,
                    (struct sockaddr*)&srv->faddr,
                    sizeof(struct sockaddr_in));
        if (nb < 0)
        {
                syslog(LOG_WARNING, "forward:sendto: %m");
                return -1;
        }
        srv->stats.up_tx += nb;

        struct timing start;
        int timeout = BHD_TIMEOUT;

        timing_start(&start);
        for (;;)
        {
                uint16_t resp_id;

                if (timeout < 1)
                {
                        syslog(LOG_WARNING, "%s:timeout waiting for response", __func__);
                        return -1;
                }

                /* Set timeout before attempting to read */
                fds[0].fd = srv->fd_forward;
                fds[0].events = POLLIN;
                ready = poll(fds, 1, timeout);
                if (ready == 0)
                {
                        /* timeout */
                        syslog(LOG_WARNING, "%s:timeout waiting for response", __func__);
                        return -1;
                }
                else if (ready < 0)
                {
                        /* error */
                        syslog(LOG_WARNING, "%s:poll:%m", __func__);
                        return -1;

                }
                nb = recvfrom(srv->fd_forward, buf, BUF_LEN, 0, NULL, NULL);
                if (nb < 0)
                {
                        syslog(LOG_WARNING, "forward:recvfrom: %m");
                        return -1;
                }
                srv->stats.up_rx += nb;

                /* Respond's id shall match request's id */
                memcpy(&resp_id, buf, 2);
                resp_id = ntohs(resp_id);
                if (h.id == resp_id)
                {
                        break;
                }
                syslog(LOG_INFO, "%s:response id mismatch", __func__);
                timeout -= (int)timing_dur_msec(&start);
        }

do_respond:
        /* Blocking of sendto(2) operations on an UDP socket is very unlikely
           to happen, so currently we omit calling poll(2). */
        nb = sendto(srv->fd_listen,
                    buf,
                    nb,
                    0,
                    (struct sockaddr*)&caddr,
                    sizeof(caddr));
        if (nb < 0)
        {
                syslog(LOG_WARNING, "client:sendto: %m");
                return -1;
        }

        srv->stats.down_tx += nb;
        bhd_dns_q_section_free(&qs);

        return 0;
}


static int bhd_srv_serve_stats(struct bhd_srv* srv)
{
        unsigned char buf[BUF_LEN];
        struct sockaddr_in caddr;
        ssize_t nb;
        socklen_t slen = sizeof(caddr);

        nb = recvfrom(srv->fd_stats,
                      buf,
                      BUF_LEN,
                      0,
                      (struct sockaddr*)&caddr,
                      &slen);
        if (nb < 0)
        {
                syslog(LOG_WARNING, "client:recvfrom: %m");
                return -1;
        }
        if (nb < 5)
        {
                return 0;
        }
        for (int i = 0; i < 5; i++)
        {
                buf[i] = (unsigned char)tolower(buf[i]);
        }
        if (strncmp((char*)&buf[0], "stats", 5))
        {
                return 0;
        }

        nb = bhd_srv_stat_str((char*)&buf[0], BUF_LEN, &srv->stats);
        nb = sendto(srv->fd_stats,
                    buf,
                    nb,
                    0,
                    (struct sockaddr*)&caddr,
                    sizeof(caddr));
        if (nb < 0)
        {
                syslog(LOG_WARNING, "client:sendto: %m");
                return -1;
        }

        return 0;
}

int bhd_srv_stat_str(char* buf, int len, const struct bhd_stats* stats)
{
        int nb = 0;

        nb += snprintf(buf+nb, len - nb, "requests.block:%ld\n", stats->numb);
        nb += snprintf(buf+nb, len - nb, "requests.forward:%ld\n", stats->numf);
        nb += snprintf(buf+nb, len - nb, "upstream.tx:%ld\n", stats->up_tx);
        nb += snprintf(buf+nb, len - nb, "upstream.rx:%ld\n", stats->up_rx);
        nb += snprintf(buf+nb, len - nb, "downstream.tx:%ld\n", stats->down_tx);
        nb += snprintf(buf+nb, len - nb, "downsteram.rx:%ld\n", stats->down_rx);

        return nb;
}

static void sigh(int signum)
{
        (void)signum;
        run = 0;
}
