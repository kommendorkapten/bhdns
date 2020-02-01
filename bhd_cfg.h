/*
* Copyright (C) 2020 Fredrik Skogman, skogman - at - gmail.com.
*
* The contents of this file are subject to the terms of the Common
* Development and Distribution License (the "License"). You may not use this
* file except in compliance with the License. You can obtain a copy of the
* License at http://opensource.org/licenses/CDDL-1.0. See the License for the
* specific language governing permissions and limitations under the License.
* When distributing the software, include this License Header Notice in each
* file and include the License file at http://opensource.org/licenses/CDDL-1.0.
*/

#ifndef BHD_CFG_H
#define BHD_CFG_H

#include <stdint.h>

#define STR_LEN 128

struct bhd_cfg
{
        char laddr[STR_LEN];
        char faddr[STR_LEN];
        char baddr[STR_LEN];
        char bp[STR_LEN];
        char user[STR_LEN];
        uint16_t lport;
        uint16_t fport;
        uint16_t sport;
};

/**
 * Open a config file for reading, fill create a cfg struct and return.
 * @params config struct to populate.
 * @params path to the file to open.
 * @return 0 on sucess.
 */
int bhd_cfg_read(struct bhd_cfg*, const char*);

#endif/* BHD_CFG_H */
