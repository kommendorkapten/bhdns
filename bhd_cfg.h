#ifndef __BHD_CFG_H__
#define __BHD_CFG_H__

#include <stdint.h>

#define STR_LEN 256

struct bhd_cfg
{
        char ifa[STR_LEN];
        char bp[STR_LEN];
        char bresp[STR_LEN];
        uint16_t port;
};

/**
 * Open a config file for reading, fill create a cfg struct and return.
 * @params path to the file to open.
 * @return a bhd_cfg struct created on the heap. NULL if errors occured.
 */
struct bhd_cfg* bhd_cfg_read(const char*);

#endif/* __BHD_CFG_H__ */
