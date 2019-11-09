#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bhd_cfg.h"
#include "lib/strutil.h"

#define MAX_LINE 512

struct bhd_cfg* bhd_cfg_read(const char* p)
{
        char line[MAX_LINE];
        struct bhd_cfg* cfg = NULL;
        FILE* f;

        f = fopen(p, "r");
        if (!f)
        {
                printf("Failed to open '%s': \n", p);
                perror("open");
                return NULL;
        }

        cfg = malloc(sizeof(struct bhd_cfg));
        if (!cfg)
        {
                perror("malloc");
                return NULL;
        }
        memset(cfg, 0, sizeof(struct bhd_cfg));

        int ln = 0;
        while (fgets(line, MAX_LINE, f))
        {
                char* d;

                ln++;
                line[MAX_LINE-1] = '\0';
                strrstrip(line);
                strlstrip(line);

                if (line[0] == '#')
                {
                        continue;
                }
                if (line[0] == '\0')
                {
                        continue;
                }

                d = strchr(line, ':');
                if (d == NULL)
                {
                        printf("Bad line (%d) '%s'\n", ln, line);
                        continue;
                }
                *d++ = '\0';
                strrstrip(line);
                strlstrip(d);

                size_t vlen = strlen(d) + 1;

                if (strncmp("port", line, vlen) == 0)
                {
                        long lv;
                        char* ep;

                        if (cfg->port)
                        {
                                printf("Multiple port declarations at line %d\n",
                                       ln);
                                continue;
                        }

                        lv = strtol(d, &ep, 10);
                        if (d == ep)
                        {
                                printf("Invalid port number %s at line %d\n",
                                       d,
                                       ln);
                                continue;
                        }
                        cfg->port = (uint16_t)lv;
                }
                else if (strncmp("if", line, vlen) == 0)
                {
                        if (cfg->ifa[0])
                        {
                                printf("Multiple if declarations at line %d\n",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->ifa, d, vlen);
                        for (size_t i = 0; i < vlen; i++)
                        {
                                cfg->ifa[i] = (char)tolower(cfg->ifa[i]);
                        }
                }
                else if (strncmp("blist", line, vlen) == 0)
                {
                        if (cfg->bp[0])
                        {
                                printf("Multiple blist declarations at line %d\n",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->bp, d, vlen);
                }
                else if (strncmp("bresp", line, vlen) == 0)
                {
                        if (cfg->bresp[0])
                        {
                                printf("Multiple bresp declarations at line %d\n",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->bresp, d, vlen);
                }

        }

        if (!cfg->port)
        {
                cfg->port = 53;
        }
        if (!cfg->ifa[0])
        {
                strncpy(cfg->ifa, "all", STR_LEN-1);
        }
        if (!cfg->bp[0])
        {
                strncpy(cfg->bp, "/var/bhdns/blist", STR_LEN-1);
        }
        if (!cfg->bresp[0])
        {
                strncpy(cfg->bresp, "0.0.0.0", STR_LEN-1);
        }

        return cfg;
}
