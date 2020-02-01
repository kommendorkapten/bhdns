#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include "bhd_cfg.h"
#include "vendor/strutil.h"

#define MAX_LINE 512

int bhd_cfg_read(struct bhd_cfg* cfg, const char* p)
{
        char line[MAX_LINE];
        FILE* f;

        f = fopen(p, "r");
        if (!f)
        {
                syslog(LOG_ERR, "%s:open '%s': %m", __func__, p);
                return -1;
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

                /* Skip comments and empty lines */
                if (line[0] == '#' || line[0] == '\0')
                {
                        continue;
                }

                d = strchr(line, ':');
                if (d == NULL)
                {
                        syslog(LOG_WARNING, "Bad line (%d) '%s'", ln, line);
                        continue;
                }
                *d++ = '\0';
                strrstrip(line);
                strlstrip(d);

                size_t vlen = strlen(d) + 1;
                size_t slen = strlen(line);

                if (strncmp("listen-port", line, slen) == 0)
                {
                        long lv;
                        char* ep;

                        if (cfg->lport)
                        {
                                syslog(LOG_WARNING,
                                       "Multiple listen-port declarations at line %d",
                                       ln);
                                continue;
                        }

                        lv = strtol(d, &ep, 10);
                        if (d == ep)
                        {
                                syslog(LOG_WARNING,
                                       "Invalid listen-port number %s at line %d",
                                       d,
                                       ln);
                                continue;
                        }
                        cfg->lport = (uint16_t)lv;
                }
                else if (strncmp("listen-addr", line, slen) == 0)
                {
                        if (cfg->laddr[0])
                        {
                                syslog(LOG_WARNING,
                                       "Multiple listen-addr declarations at line %d",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->laddr, d, vlen);
                }
                else if (strncmp("blist", line, slen) == 0)
                {
                        if (cfg->bp[0])
                        {
                                syslog(LOG_WARNING,
                                       "Multiple blist declarations at line %d",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->bp, d, vlen);
                }
                else if (strncmp("bresp", line, slen) == 0)
                {
                        if (cfg->baddr[0])
                        {
                                syslog(LOG_WARNING,
                                       "Multiple bresp declarations at line %d",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->baddr, d, vlen);
                }
                else if (strncmp("forward-addr", line, slen) == 0)
                {
                        if (cfg->faddr[0])
                        {
                                syslog(LOG_WARNING,
                                       "Multiple forward-addr declarations at line %d",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->faddr, d, vlen);
                }
                else if (strncmp("forward-port", line, slen) == 0)
                {
                        long lv;
                        char* ep;

                        if (cfg->fport)
                        {
                                syslog(LOG_WARNING,
                                       "Multiple forward-port declarations at line %d",
                                       ln);
                                continue;
                        }

                        lv = strtol(d, &ep, 10);
                        if (d == ep)
                        {
                                syslog(LOG_WARNING,
                                       "Invalid forward-port number %s at line %d",
                                       d,
                                       ln);
                                continue;
                        }
                        cfg->fport = (uint16_t)lv;
                }
                else if (strncmp("stats-port", line, slen) == 0)
                {
                        long lv;
                        char* ep;

                        if (cfg->sport)
                        {
                                syslog(LOG_WARNING,
                                       "Multiple stats-port declarations at line %d",
                                       ln);
                                continue;
                        }

                        lv = strtol(d, &ep, 10);
                        if (d == ep)
                        {
                                syslog(LOG_WARNING,
                                       "Invalid stats-port number %s at line %d",
                                       d,
                                       ln);
                                continue;
                        }
                        cfg->sport = (uint16_t)lv;
                }
                else if (strncmp("user", line, slen) == 0)
                {
                        if (cfg->user[0])
                        {
                                syslog(LOG_WARNING,
                                       "Multiple user declarations at line %d",
                                       ln);
                                continue;
                        }
                        strncpy(cfg->user, d, vlen);
                }

        }

        if (!cfg->lport)
        {
                cfg->lport = 53;
        }
        if (!cfg->fport)
        {
                cfg->fport = 53;
        }
        if (!cfg->laddr[0])
        {
                strncpy(cfg->laddr, "0.0.0.0", STR_LEN-1);
        }
        if (!cfg->bp[0])
        {
                strncpy(cfg->bp, "/var/bhdns/blist", STR_LEN-1);
        }
        if (!cfg->baddr[0])
        {
                strncpy(cfg->baddr, "0.0.0.0", STR_LEN-1);
        }

        fclose(f);

        return 0;
}
