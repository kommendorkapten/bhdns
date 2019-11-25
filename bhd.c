#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include "bhd_cfg.h"
#include "bhd_srv.h"
#include "bhd_bl.h"

int main(int argc, char** argv)
{
        char* cfgp = "/etc/bhdns";
        struct bhd_cfg cfg;
        struct bhd_bl* bl;
        int d = 0;
        int c;

        openlog("bhdns", LOG_CONS|LOG_NDELAY, LOG_USER);

        while ((c = getopt(argc, argv, "df:")) != -1)
        {
                switch(c)
                {
                case 'd':
                        d = 1;
                        break;
                case 'f':
                        cfgp = optarg;
                        break;
                default:
                        syslog(LOG_ERR, "unknown parameter: %c", c);
                        return 1;
                }
        }

        c = bhd_cfg_read(&cfg, cfgp);
        if (c)
        {
                syslog(LOG_ERR, "No configuration found, exiting");
                return 1;
        }

        syslog(LOG_INFO, "Starting: daemon %d", d);
        bl = bhd_bl_create(cfg.bp);
        bhd_serve(&cfg, bl);
        syslog(LOG_INFO, "Stopping");
        bhd_bl_free(bl);

        return 0;
}
