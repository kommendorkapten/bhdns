#ifndef __BHD_SRV_H__
#define __BHD_SRV_H__

/**
 * Start server.
 * @param listen address, IPv4 dot notation, or 'all'.
 * @param port to listen to.
 * @return -1 if server unexpectedly stopped.
 */
int bhd_serve(const char*, uint16_t);

#endif /* __BHD_SRV_H__ */
