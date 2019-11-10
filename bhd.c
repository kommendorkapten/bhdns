#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bhd_cfg.h"
#include "bhd_srv.h"

int main(int argc, char** argv)
{
        char* cfgp = "/etc/bhdns";
        struct bhd_cfg* cfg;
        int d = 0;
        int c;

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
                        printf("unknown parameter: %c\n", c);
                        return 1;
                }
        }

        cfg = bhd_cfg_read(cfgp);
        if (!cfg)
        {
                printf("No configuration found");
                return 1;
        }

        bhd_serve(cfg->faddr, cfg->ifa, cfg->port);
        free(cfg);
        return 0;
}