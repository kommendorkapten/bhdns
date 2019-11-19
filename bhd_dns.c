#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "bhd_dns.h"

size_t bhd_dns_h_unpack(struct bhd_dns_h* h, const unsigned char* buf)
{
        uint16_t u16;
        uint8_t u8;

        memcpy(&u16, buf, 2);
        h->id = ntohs(u16);

        memcpy(&u8, buf + 2, 1);
        h->qr = (uint8_t)((u8 & 0x80) >> 7);
        h->opcode = (uint8_t)((u8 & 0x78) >> 3);
        h->tc = (uint8_t)((u8 & 0x2) >> 1);
        h->rd = (uint8_t)((u8 & 0x1));

        memcpy(&u8, buf + 3, 1);
        h->ra = (uint8_t)((u8 & 0x80) >> 7);
        h->z1 = (uint8_t)((u8 & 0x40) >> 6);
        h->ad = (uint8_t)((u8 & 0x20) >> 5);
        h->cd = (uint8_t)((u8 & 0x10) >> 4);
        h->rcode = (uint8_t)(u8 & 0xf);

        memcpy(&u16, buf + 4, 2);
        h->qd_count = ntohs(u16);

        memcpy(&u16, buf + 6, 2);
        h->an_count = ntohs(u16);

        memcpy(&u16, buf + 8, 2);
        h->ns_count = ntohs(u16);

        memcpy(&u16, buf + 10, 2);
        h->ar_count = ntohs(u16);

        return BHD_DNS_H_SIZE;
}

size_t bhd_dns_h_pack(unsigned char* buf,
                      size_t len,
                      const struct bhd_dns_h* h)
{
        uint16_t u16;
        uint8_t u8;

        if (len < BHD_DNS_H_SIZE)
        {
                return 0;
        }

        u16 = htons(h->id);
        memcpy(buf, &u16, 2);

        u8 = 0;
        u8 |= (uint8_t)((h->qr & 0x1) << 7);
        u8 |= (uint8_t)((h->opcode & 0xf) << 3);
        u8 |= (uint8_t)((h->tc & 0x1) << 1);
        u8 |= (uint8_t)(h->rd & 0x1);
        memcpy(buf + 2, &u8, 1);

        u8 = 0;
        u8 |= (uint8_t)((h->ra & 0x1) << 7);
        u8 |= (uint8_t)((h->z1 & 0x1) << 6);
        u8 |= (uint8_t)((h->ad & 0x1) << 5);
        u8 |= (uint8_t)((h->cd & 0x1) << 4);
        u8 |= (uint8_t)(h->rcode & 0xf);
        memcpy(buf + 3, &u8, 1);

        u16 = htons(h->qd_count);
        memcpy(buf + 4, &u16, 2);

        u16 = htons(h->an_count);
        memcpy(buf + 6, &u16, 2);

        u16 = htons(h->ns_count);
        memcpy(buf + 8, &u16, 2);

        u16 = htons(h->ar_count);
        memcpy(buf + 10, &u16, 2);

        return BHD_DNS_H_SIZE;
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
                printf("WARN: could not allocate memory\n");
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
        uint16_t v;
        uint8_t len;

        /* See RFC 1035 4.1.2 for format */
        /* Format is 3www7openbsd3org */
        len = *buf;
        if (len > BHD_DNS_MAX_LABEL)
        {
                return 0;
        }
        br++;
        for (;;)
        {
                label->label = malloc(len + 1);
                if (!label->label)
                {
                        printf("WARN:could not allocate memory\n");
                        goto bailout;
                }
                memcpy(label->label, buf + br, len);
                label->label[len] = '\0';

                br += len;
                len = *(buf + br++);
                if (len > BHD_DNS_MAX_LABEL)
                {
                        goto bailout;
                }
                else if (len == 0)
                {
                        label->next = NULL;
                        break;
                }
                label->next = malloc(sizeof(struct bhd_dns_q_label));
                if (!label->next)
                {
                        printf("WARN:could not allocate memory\n");
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
        printf("id: %d\n", h->id);

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
                printf("label: %s\n",label->label);
        }
        printf("\nqtype: %d\nqclass: %d\n", q->qtype, q->qclass);
}
