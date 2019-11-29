#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <poll.h>
#include "bhd_srv.h"
#include "bhd_dns.h"
#include "bhd_bl.h"
#include "bhd_cfg.h"

/* Max UDP message size from RFC1035 */
#define BUF_LEN 512
/* Default timeout in ms */
#define BHD_TIMEOUT 5000
sig_atomic_t run;

void bhd_dns_rr_a_init(struct bhd_dns_rr_a*, const char*);

/**
 * Return 0 if successful.
 */
static int bhd_srv_serve_one(int,
                             int,
                             struct sockaddr_in*,
                             struct bhd_bl*,
                             struct bhd_stats*,
                             const char*);

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
        srv->daemon = daemon;

        run = 0;

        sa.sa_flags = 0;
        sa.sa_handler = &sigh;
        sigfillset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL))
        {
                syslog(LOG_WARNING, "Failed to install sighandler %m");
        }

        /* Set up forward address */
        syslog(LOG_INFO, "Forward address: %s", cfg->faddr);
        srv->fd_forward = socket(AF_INET, SOCK_DGRAM, 0);
        if (srv->fd_forward < 0)
        {
                syslog(LOG_ERR, "Could not create socket: %m");
                return -1;
        }
        memset(&srv->faddr, 0, sizeof(srv->faddr));
        srv->faddr.sin_family = AF_INET;
        srv->faddr.sin_port = htons(53);
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
        saddr.sin_port = htons(cfg->port);
        if (strncmp(cfg->ifa, "all", 3) == 0)
        {
                saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
                uint32_t na;

                if (inet_pton(AF_INET, cfg->ifa, &na) < 0)
                {
                        syslog(LOG_ERR, "Invalid address '%s': %m", cfg->ifa);
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

        return 0;
}

int bhd_serve(struct bhd_srv* srv)
{
        int ret;

        run = 1;
        while(run)
        {
                ret = bhd_srv_serve_one(srv->fd_listen,
                                        srv->fd_forward,
                                        &srv->faddr,
                                        srv->bl,
                                        &srv->stats,
                                        srv->cfg->baddr);
                if (ret)
                {
                        syslog(LOG_WARNING, "DNS action failed");
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

static int bhd_srv_serve_one(int s,
                             int fs,
                             struct sockaddr_in* faddr,
                             struct bhd_bl* bl,
                             struct bhd_stats* stats,
                             const char* baddr)
{
        unsigned char buf[BUF_LEN];
        struct sockaddr_in caddr;
        struct bhd_dns_h h;
        struct bhd_dns_q_section qs;
        struct pollfd fds[1];
        size_t br;
        size_t offset = 0;
        ssize_t nb;
        socklen_t slen = sizeof(caddr);
        int ready;

        nb = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr*)&caddr, &slen);
        if (nb < 0)
        {
                syslog(LOG_WARNING, "client:recvfrom: %m");
                return -1;
        }

        stats->down_rx += nb;
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

#if DEBUG
        bhd_dns_h_dump(&h);
#endif
        /* If ad flag is set, ignore additional data */
        if (offset != (size_t)nb && h.ad == 0)
        {
#if DEBUG
                for (int i = offset; i < nb; i++)
                {
                        printf("%02x:", buf[i]);
                }
                printf("\n");
#endif
                syslog(LOG_WARNING,
                       "Not all data was unpacked: got %d want %d",
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
                int m = bhd_bl_match(bl, l);

                if (m)
                {
                        /* Send static response */
                        struct bhd_dns_rr_a rr;

                        bhd_dns_rr_a_init(&rr, baddr);
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

                        stats->numb++;
                        goto do_respond;
                }

        }

        stats->numf++;

        /* Blocking of sendto(2) operations on an UDP socket is very unlikely
           to happen, so currently we omit calling poll(2). */
        nb = sendto(fs, buf, nb, 0, (struct sockaddr*)faddr, sizeof(struct sockaddr_in));
        if (nb < 0)
        {
                syslog(LOG_WARNING, "forward:sendto: %m");
                return -1;
        }
        stats->up_tx += nb;

        /* Set timeout before attempting to read */
        fds[0].fd = fs;
        fds[0].events = POLLIN;
        ready = poll(fds, 1, BHD_TIMEOUT);
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
        nb = recvfrom(fs, buf, BUF_LEN, 0, NULL, NULL);
        if (nb < 0)
        {
                syslog(LOG_WARNING, "forward:recvfrom: %m");
                return -1;
        }
        stats->up_rx += nb;

do_respond:
        /* Blocking of sendto(2) operations on an UDP socket is very unlikely
           to happen, so currently we omit calling poll(2). */
        nb = sendto(s, buf, nb, 0, (struct sockaddr*)&caddr, sizeof(caddr));
        if (nb < 0)
        {
                syslog(LOG_WARNING, "client:sendto: %m");
                return -1;
        }

        stats->down_tx += nb;
        bhd_dns_q_section_free(&qs);

        return 0;
}

void bhd_dns_rr_a_init(struct bhd_dns_rr_a* rr, const char* a)
{
        uint32_t na;

        if (inet_pton(AF_INET, a, &na) < 0)
        {
                syslog(LOG_WARNING, "Invalid address '%s': %m", a);
        }

        /* two MSB bits to 11, and offset of 12 */
        /* 11000000 00001100 */
        rr->name = 0xc00c;
        rr->type = (uint16_t)BHD_DNS_QTYPE_A;
        rr->class = (uint16_t)BHD_DNS_CLASS_IN;
        rr->ttl = 86400; /* 24h */
        rr->rdlength = 4;
        rr->addr = na;
}

static void sigh(int signum)
{
        (void)signum;
        run = 0;
}
