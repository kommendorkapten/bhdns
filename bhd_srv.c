#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "bhd_srv.h"
#include "bhd_dns.h"

#define BUF_LEN 1432

int bhd_serve(const char* addr, uint16_t port)
{
        char buf[BUF_LEN];
        ssize_t nb;
        socklen_t slen;
        int s;
        int ret;
        struct sockaddr_in saddr;
        struct sockaddr_in caddr;

        if (strncmp(addr, "all", 3))
        {
                printf("Only all address is supported\n");
                return -1;
        }

        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
        {
                perror("socket");
                return -1;
        }

        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        saddr.sin_port = htons(port);

        ret = bind(s, (struct sockaddr*)&saddr, sizeof(saddr));
        if (ret < 0)
        {
                perror("bind");

                return -1;
        }

        printf("header size: %ld\n", sizeof(struct bhd_dns_h));

        nb = recvfrom(s, buf, BUF_LEN, 0, (struct sockaddr*)&caddr, &slen);
        if (nb < 0)
        {
                perror("recvfrom");
                return -1;
        }
        printf("Read %ld bytes from socket\n", nb);

        struct bhd_dns_h h;
        struct bhd_dns_q_section qs;
        size_t br;
        size_t tmp;

        br = bhd_dns_h_unpack(&h, buf);
        bhd_dns_h_dump(&h);

        qs.qd_count = h.qd_count;
        tmp = bhd_dns_q_section_unpack(&qs, buf + br);
        if (tmp == 0)
        {
                abort();
        }
        br += tmp;

        bhd_dns_q_section_dump(&qs);
        printf("Unpacked %ld bytes\n", br);

        if (br != (size_t)nb)
        {
                printf("WARN:Did not unpack all data!\n");
        }

        bhd_dns_q_section_free(&qs);

        return 1;
}
