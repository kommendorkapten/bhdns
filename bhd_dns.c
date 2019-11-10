#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "bhd_dns.h"

uint8_t bdh_ntohb(uint8_t v)
{
        uint8_t r;

        r = (uint8_t)(
                (v & 0x1) << 7 |
                (v & 0x2) << 5 |
                (v & 0x4) << 3 |
                (v & 0x8) << 1 |
                (v & 0x10) >> 1 |
                (v & 0x20) >> 3 |
                (v & 0x40) >> 5 |
                (v & 0x80) >> 7);
        return r;
}

struct bhd_dns_h_n
{
        uint16_t hw1;
        uint8_t hw20;
        uint8_t hw21;
        uint16_t hw3;
        uint16_t hw4;
        uint16_t hw5;
        uint16_t hw6;
};

void bhd_dns_h_n_unpack(struct bhd_dns_h_n*, const unsigned char*);
void bhd_dns_h_n_pack(unsigned char*, const struct bhd_dns_h_n*);

void bhd_dns_h_n_unpack(struct bhd_dns_h_n* n, const unsigned char* buf)
{
        memcpy(&n->hw1, buf + 0, 2);
        memcpy(&n->hw20, buf + 2, 1);
        memcpy(&n->hw21, buf + 3, 1);
        memcpy(&n->hw3, buf + 4, 2);
        memcpy(&n->hw4, buf + 6, 2);
        memcpy(&n->hw5, buf + 8, 2);
        memcpy(&n->hw6, buf + 10, 2);
}

void bhd_dns_h_n_pack(unsigned char* buf, const struct bhd_dns_h_n* n)
{
        memcpy(buf + 0, &n->hw1, 2);
        memcpy(buf + 2, &n->hw20, 1);
        memcpy(buf + 3, &n->hw21, 1);
        memcpy(buf + 4, &n->hw3, 2);
        memcpy(buf + 6, &n->hw4, 2);
        memcpy(buf + 8, &n->hw5, 2);
        memcpy(buf + 10, &n->hw6, 2);
}

size_t bhd_dns_h_unpack(struct bhd_dns_h* h, const unsigned char* buf)
{
        struct bhd_dns_h_n nh;

        assert(sizeof(struct bhd_dns_h_n) == sizeof(struct bhd_dns_h));
        bhd_dns_h_n_unpack(&nh, buf);

        nh.hw1 = ntohs(nh.hw1);
        nh.hw20 = bdh_ntohb(nh.hw20);
        nh.hw21 = bdh_ntohb(nh.hw21);
        nh.hw3 = ntohs(nh.hw3);
        nh.hw4 = ntohs(nh.hw4);
        nh.hw5 = ntohs(nh.hw5);
        nh.hw6 = ntohs(nh.hw6);

        memcpy(h, &nh, sizeof(nh));

        return sizeof(nh);
}

size_t bhd_dns_h_pack(unsigned char* buf,
                      size_t len,
                      const struct bhd_dns_h* h)
{
        struct bhd_dns_h_n nh;

        if (len < sizeof(struct bhd_dns_h))
        {
                return 0;
        }

        assert(sizeof(struct bhd_dns_h_n) == sizeof(struct bhd_dns_h));
        memcpy(&nh, h, sizeof(nh));

        nh.hw1 = ntohs(nh.hw1);
        nh.hw20 = bdh_ntohb(nh.hw20);
        nh.hw21 = bdh_ntohb(nh.hw21);
        nh.hw3 = ntohs(nh.hw3);
        nh.hw4 = ntohs(nh.hw4);
        nh.hw5 = ntohs(nh.hw5);
        nh.hw6 = ntohs(nh.hw6);

        bhd_dns_h_n_pack(buf, &nh);

        return sizeof(nh);
}

size_t bhd_dns_q_section_unpack(struct bhd_dns_q_section* qs,
                                const unsigned char* buf)
{
        size_t offset = 0;

        if (qs->qd_count == 0)
        {
                return 0;
        }

        qs->q = malloc(sizeof(struct bhd_dns_q) * qs->qd_count);
        if (!qs->q)
        {
                return 0;
        }

        for (uint16_t i = 0; i < qs->qd_count; i++)
        {
                size_t br = bhd_dns_q_unpack(qs->q + i, buf + offset);

                if (br == 0)
                {
                        qs->qd_count = i;
                        bhd_dns_q_section_free(qs);
                        return 0;
                }
                offset += br;
        }

        return offset;
}

size_t bhd_dns_q_section_pack(unsigned char* buf,
                              size_t len,
                              const struct bhd_dns_q_section* qs)
{
        size_t offset = 0;

        for (uint16_t i = 0; i < qs->qd_count; i++)
        {
                offset += bhd_dns_q_pack(buf + offset, len - offset, qs->q + i);
        }

        return offset;
}

size_t bhd_dns_rr_a_pack(unsigned char* buf,
                         size_t len,
                         const struct bhd_dns_rr_a* a)
{
        uint32_t u32;
        uint16_t u16;

        if (len < 16)
        {
                return 0;
        }

        u16= htons(a->name);
        memcpy(buf + 0, &u16, 2);
        u16 = htons(a->type);
        memcpy(buf + 2, &u16, 2);
        u16 = htons(a->class);
        memcpy(buf + 4, &u16, 2);
        u32 = htonl(a->ttl);
        memcpy(buf + 6, &u32, 4);
        u16 = htons(a->rdlength);
        memcpy(buf + 10, &u16, 2);
        /* addr is always in network order from inet_pton */
        memcpy(buf + 12, &a->addr, 4);

        return 16;
}

void bhd_dns_q_section_free(struct bhd_dns_q_section* qs)
{
        for (int i = 0; i < qs->qd_count; i++)
        {
                struct bhd_dns_q_label* l = &qs->q[i].qname;

                free(l->label);
                l = l->next;
                while (l)
                {
                        struct bhd_dns_q_label* next = l->next;

                        free(l->label);
                        free(l);
                        l = next;
                }
        }

        free(qs->q);
        qs->q = NULL;
        qs->qd_count = 0;
}

size_t bhd_dns_q_unpack(struct bhd_dns_q* q, const unsigned char* buf)
{
        struct bhd_dns_q_label* label = &q->qname;
        size_t br = 0;
        uint8_t len;
        uint16_t v;

        /* See RFC 1035 4.1.2 for format */
        /* Format is 3www7openbsd3org */
        len = *buf;
        br++;
        for (;;)
        {
                label->label = malloc(len + 1);
                if (!label->label)
                {
                        goto bailout;
                }
                memcpy(label->label, buf + br, len);
                label->label[len] = '\0';

                br += len;
                len = *(buf + br++);

                if (len == 0)
                {
                        label->next = NULL;
                        break;
                }
                label->next = malloc(sizeof(struct bhd_dns_q_label));
                if (!label->next)
                {
                        goto bailout;
                }
                label = label->next;
        }

        memcpy(&v, buf + br, 2);
        q->qtype = ntohs(v);
        br += 2;
        memcpy(&v, buf + br, 2);
        q->qclass = ntohs(v);
        br += 2;

        return br;
bailout:
        label = &q->qname;
        if (label->label)
        {
                free(label->label);
                label = label->next;
                while (label)
                {
                        struct bhd_dns_q_label* next = label->next;

                        free(label->label);
                        free(label);
                        label = next;
                }
        }
        return 0;
}

size_t bhd_dns_q_pack(unsigned char* buf, size_t len, const struct bhd_dns_q* q)
{
        /* Max length of a single domain */
        char tmp[256];
        size_t offset = 0;
        const struct bhd_dns_q_label* label = &q->qname;
        uint16_t v;

        while (label)
        {
                int p = 0;
                size_t len = strlen(label->label);
                if (len > 255)
                {
                        abort();
                }

                tmp[offset++] = (uint8_t)len;
                while (label->label[p])
                {
                        tmp[offset++] = label->label[p++];
                }
                label = label->next;
        }

        if (len < offset + 4)
        {
                return 0;
        }

        memcpy(buf, tmp, offset);
        buf[offset++] = '\0';
        v = htons(q->qtype);
        memcpy(buf + offset, &v, 2);
        offset += 2;
        v = htons(q->qclass);
        memcpy(buf + offset, &v, 2);
        offset += 2;

        return offset;
}

void bhd_dns_h_dump(const struct bhd_dns_h* h)
{
        struct bhd_dns_h_n nh;

        memcpy(&nh, h, sizeof(nh));

        printf("id: %d\n", h->id);
        printf("20: 0x%x\n", nh.hw20);
        printf("21: 0x%x\n", nh.hw21);

        printf("qr: %d\n", h->qr);
        printf("opcode: %d\n", h->opcode);
        printf("aa: %d\n", h->aa);
        printf("tc: %d\n", h->tc);
        printf("rd: %d\n", h->rd);
        printf("\n");

        printf("ra: %d\n", h->ra);
        printf("z1: %d\n", h->z1);
        printf("ad: %d\n", h->ad);
        printf("cd: %d\n", h->cd);
        printf("rcode: %d\n", h->rcode);
        printf("\n");

        printf("qd count: %d\n", h->qd_count);
        printf("an count: %d\n", h->an_count);
        printf("ns count: %d\n", h->ns_count);
        printf("ar count: %d\n", h->ar_count);

}

void bhd_dns_q_section_dump(const struct bhd_dns_q_section* qs)
{
        for (int i = 0; i < qs->qd_count; i++)
        {
                printf("Query: %d/%d\n", i + 1, qs->qd_count);
                bhd_dns_q_dump(qs->q + i);
        }
}

void bhd_dns_q_dump(const struct bhd_dns_q* q)
{
        const struct bhd_dns_q_label* label = &q->qname;

        printf("Domain: ");
        int first = 1;
        for (; label; label = label->next)
        {
                if (!first)
                {
                        printf(".");
                }
                else
                {
                        first = 0;
                }
                printf(label->label);
        }
        printf("\nqtype: %d\nqclass: %d\n", q->qtype, q->qclass);
}
