#ifndef __BHD_SRV_H__
#define __BHD_SRV_H__

struct bhd_bl;
struct bhd_cfg;

/**
 * Start server.
 * @param configuration
 * @param block list.
 * @return -1 if server unexpectedly stopped.
 */
int bhd_serve(const struct bhd_cfg*,  struct bhd_bl*);

#endif /* __BHD_SRV_H__ */
