#ifndef __BHD_DNS_H__
#define __BHD_DNS_H__

#include <stdint.h>

enum bhd_dns_h_opcode
{
        BHD_DNS_OP_QUERY = 0,
        BHD_DNS_OP_IQUERY = 1,
        BHD_DNS_OP_STATUS = 2,
};

enum bhd_dns_h_qtype
{
        /* type subset */
        BHD_DNS_QTYPE_A = 1,
        BHD_DNS_QTYPE_NS = 2,
        BHD_DNS_QTYPE_CNAME = 5,
        BHD_DNS_QTYPE_SOA = 6,
        BHD_DNS_QTYPE_WKS = 11,
        BHD_DNS_QTYPE_PTR = 12,
        BHD_DNS_QTYPE_HINFO = 13,
        BHD_DNS_QTYPE_MINFO = 14,
        BHD_DNS_QTYPE_MX = 15,
        BHD_DNS_QTYPE_TXT = 16,
        /* qtype elements */
        BHD_DNS_QTYPE_AXFR = 252,
        BHD_DNS_QTYPE_MAILA = 254,
        BHD_DNS_QTYPE_ALL = 255,
};

enum bhd_dns_h_qclass
{
        /* class subset */
        BHD_DNS_CLASS_IN = 1,
        BHD_DNS_CLASS_CH = 3,
        BHD_DNS_CLASS_HS = 4,
        /* qclass elements */
        BHD_DNS_CLASS_ANY = 255,
};

/* https://tools.ietf.org/html/rfc1035#section-4.1.1 */
struct bhd_dns_h
{
        /* 0 - 15 */
        /* identification number */
        uint16_t id;

        /* 16 - 23 */
        /* query/response flag */
        uint8_t qr :1;
        /* opcode */
        uint8_t opcode :4;
        /* authoritive answer */
        uint8_t aa :1;
        /* truncated message */
        uint8_t tc :1;
        /* recursion desired */
        uint8_t rd :1;

        /* 24 - 31 */
        /* recursion available */
        uint8_t ra :1;
        /* reserved, set to 0 */
        uint8_t z1 :1;
        /* authenticated data */
        uint8_t ad :1;
        /* checking disabled */
        uint8_t cd :1;
        /* response code */
        uint8_t rcode :4;

        /* 32 - 47 */
        /* num entries in question section */
        uint16_t qd_count;
        /* 48 - 63 */
        /* num resource records in answer section */
        uint16_t an_count;

        /* 64 - 79 */
        /* num of name server resource records in authority section */
        uint16_t ns_count;

        /* 80 - 95 */
        /* num resource records in additional records section */
        uint16_t ar_count;
};

/* each label is max 63 byte, and in todal the domain < 255 */

struct bhd_dns_q_label;
struct bhd_dns_q_label
{
        char* label;
        struct bhd_dns_q_label* next;
};

struct bhd_dns_q
{

        struct bhd_dns_q_label qname;
        uint16_t qtype;
        uint16_t qclass;
};

struct bhd_dns_q_section
{
        struct bhd_dns_q* q;
        uint16_t qd_count;
};

struct bhd_dns_rr_a
{
        uint16_t name;
        uint16_t type;
        uint16_t class;
        uint32_t ttl;
        uint16_t rdlength;
        uint32_t addr;
};

/**
 * Unpack a DNS header from the provided buffer.
 * @param bhd_dns_h struct to populate.
 * @param buffer to read from.
 * @return number of bytes read from the buffer.
 */
size_t bhd_dns_h_unpack(struct bhd_dns_h*, const unsigned char*);

/**
 * Write a dss heade to a buffer.
 * @param buffer to write to.
 * @param size of buffer in bytes.
 * @param dns header to write.
 * @return number of bytes written.
 */
size_t bhd_dns_h_pack(unsigned char*, size_t, const struct bhd_dns_h*);

/**
 * Unpack provided number of query sections. The q pointer must not point
 * to any referenced memory as it will be overwritten.
 * @param query section struct with qd_count populated.
 * @param buffer to read from.
 * @return number of bytes read from the buffer; 0 indicates an error.
 */
size_t bhd_dns_q_section_unpack(struct bhd_dns_q_section*,
                                const unsigned char*);

/**
 * Write a dns query section to a buffer.
 * @param buffer.
 * @param size of buffer in bytes.
 * @param dns query section to write.
 * @return number of bytes written.
 */
size_t bhd_dns_q_section_pack(unsigned char*,
                              size_t,
                              const struct bhd_dns_q_section*);

/**
 * Write a dns rr A to a buffer.
 * @param buffer.
 * @param size of buffer in bytes.
 * @param rr to write.
 * @return number of bytes written.
 */
size_t bhd_dns_rr_a_pack(unsigned char*, size_t, const struct bhd_dns_rr_a*);

/**
 * Free the memory referenced by the content of the provided struct.
 * The struct itself is not freed, and q is set to NULL;.
 * @param struct to clear content from.
 * @return void;
 */
void bhd_dns_q_section_free(struct bhd_dns_q_section*);

void bhd_dns_h_dump(const struct bhd_dns_h*);

void bhd_dns_q_section_dump(const struct bhd_dns_q_section*);
void bhd_dns_q_dump(const struct bhd_dns_q*);

/* Internal functions */
size_t bhd_dns_q_unpack(struct bhd_dns_q*, const unsigned char*);
size_t bhd_dns_q_pack(unsigned char*, size_t, const struct bhd_dns_q*);
#endif /* __BHD_DNS_H__ */
