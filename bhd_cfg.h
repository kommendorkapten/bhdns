#ifndef __BHD_CFG_H__
#define __BHD_CFG_H__

#include <stdint.h>

#define STR_LEN 256

struct bhd_cfg
{
        char ifa[STR_LEN];
        char bp[STR_LEN];
        char baddr[STR_LEN];
        char faddr[STR_LEN];
        uint16_t port;
};

/**
 * Open a config file for reading, fill create a cfg struct and return.
 * @params config struct to populate.
 * @params path to the file to open.
 * @return 0 on sucess.
 */
int bhd_cfg_read(struct bhd_cfg*, const char*);

#endif/* __BHD_CFG_H__ */
