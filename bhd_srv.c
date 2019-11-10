#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "bhd_srv.h"
#include "bhd_dns.h"

/* Max UDP message size from RFC1035 */
#define BUF_LEN 512

void bhd_dns_rr_a_init(struct bhd_dns_rr_a*, const char*);
static int bhd_srv_serve_one(int, int, struct sockaddr_in*);

int bhd_serve(const char* pfaddr, const char* addr, uint16_t port)
{
        struct sockaddr_in saddr;
        struct sockaddr_in faddr;
        int s;
        int fs;
        int ret;


        /* Set up forward address */
        printf("Forward address: %s\n", pfaddr);
        memset(&faddr, 0, sizeof(faddr));
        faddr.sin_family = AF_INET;
        faddr.sin_port = htons(53);
        if (inet_pton(AF_INET, pfaddr, &faddr.sin_addr) < 0)
        {
                perror("inet_pton");
                return -1;
        }
        fs = socket(AF_INET, SOCK_DGRAM, 0);

        /* Set up listening socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
        {
                perror("socket");
                return -1;
        }
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(port);
        if (strncmp(addr, "all", 3) == 0)
        {
                saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
                uint32_t na;

                if (inet_pton(AF_INET, addr, &na) < 0)
                {
                        perror("inet_pton");
                        return -1;
                }

                saddr.sin_addr.s_addr = na;
        }
        ret = bind(s, (struct sockaddr*)&saddr, sizeof(saddr));
        if (ret < 0)
        {
                perror("bind");
                return -1;
        }

        for (;;)
        {
                bhd_srv_serve_one(s, fs, &faddr);
        }

        return 0;
}

static int bhd_srv_serve_one(int s, int fs, struct sockaddr_in* faddr)
{
        unsigned char buf[BUF_LEN];
        struct sockaddr_in caddr;
        struct bhd_dns_h h;
        struct bhd_dns_q_section qs;
        size_t br;
        ssize_t nb;
        socklen_t slen;

        nb = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr*)&caddr, &slen);
        if (nb < 0)
        {
                perror("client:recvfrom");
                return -1;
        }

        br = bhd_dns_h_unpack(&h, buf);
        qs.qd_count = h.qd_count;
        br += bhd_dns_q_section_unpack(&qs, buf + br);

        if (br != (size_t)nb)
        {
                printf("WARN:Did not unpack all data!\n");
                return -1;
        }

        nb = sendto(fs, buf, nb, 0, (struct sockaddr*)faddr, sizeof(struct sockaddr_in));
        if (nb < 0)
        {
                perror("forward:sendto");
                return -1;
        }

        nb = recvfrom(fs, buf, BUF_LEN, 0, NULL, NULL);
        if (nb < 0)
        {
                perror("forward:recvfrom");
                return -1;
        }
        nb = sendto(s, buf, nb, 0, (struct sockaddr*)&caddr, sizeof(caddr));
        if (nb < 0)
        {
                perror("client:sendto");
        }

        return 0;
}

void bhd_dns_rr_a_init(struct bhd_dns_rr_a* rr, const char* a)
{
        uint32_t na;

        if (inet_pton(AF_INET, a, &na) < 0)
        {
                perror("inet_pton");
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
