#ifndef BHD_BL_H
#define BHD_BL_H

struct bhd_bl;
struct bhd_dns_q_label;

struct bhd_bl* bhd_bl_create(const char*);

/**
 * Match provided label against the block list.
 * If complete or (sub domani) partial match is made, 1 is returned.
 * Example.
 * bad.host.com is in the block list.
 * 1) bad.host.com is matched.
 * 2) whatever.bad.host.com is matched.
 * 3) I.e .*bad.host.com is matched
 * 4) ad.host.com is not matched.
 * @param block list.
 * @param pointer to a label as present in the DNS query.
 * @return 1 if the label is present as a host or part of a subdomain.
 */
int bhd_bl_match(struct bhd_bl*, const struct bhd_dns_q_label*);
void bhd_bl_free(struct bhd_bl*);

#endif /* BLD_BL_H */
