#ifndef __BHD_SRV_H__
#define __BHD_SRV_H__

struct bhd_bl;

/**
 * Start server.
 * @param forward address, IPv4 dot notation.
 * @param listen address, IPv4 dot notation, or 'all'.
 * @param port to listen to.
 * @param block list.
 * @return -1 if server unexpectedly stopped.
 */
int bhd_serve(const char*, const char*, uint16_t, struct bhd_bl*);

#endif /* __BHD_SRV_H__ */
