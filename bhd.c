#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>
#include <fcntl.h>
#include "bhd_cfg.h"
#include "bhd_srv.h"
#include "bhd_bl.h"

void daemonize(void);

int main(int argc, char** argv)
{
        char* cfgp = "/etc/bhdns";
        struct bhd_cfg cfg;
        struct bhd_bl* bl;
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
                        printf("unknown parameter: %c", c);
                        return 1;
                }
        }

        if (d)
        {
                daemonize();
        }
        openlog("bhdns", LOG_CONS|LOG_NDELAY, LOG_USER);

        c = bhd_cfg_read(&cfg, cfgp);
        if (c)
        {
                printf("No configuration found, exiting");
                return 1;
        }
        bl = bhd_bl_create(cfg.bp);

        syslog(LOG_INFO, "Starting");
        bhd_serve(&cfg, bl);
        syslog(LOG_INFO, "Stopping");
        bhd_bl_free(bl);

        return 0;
}

/**
 * Daemonize by change working dir to /, close all fds and double fork.
 */
void daemonize(void)
{
        struct rlimit rl;
        struct sigaction sa;
        pid_t pid;
        int fd0;
        int fd1;
        int fd2;

        umask(0);

        pid = fork();
        if (pid < 0)
        {
                syslog(LOG_ERR, "%s:fork1:%m", __func__);
                exit(1);
        }
        if (pid)
        {
                exit(0);
        }
        setsid();

        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGHUP, &sa, NULL) < 0)
        {
                syslog(LOG_ERR, "%s:sigaction:%m", __func__);
                exit(1);
        }
        pid = fork();
        if (pid < 0)
        {
                syslog(LOG_ERR, "%s:fork2:%m", __func__);
                exit(1);
        }
        if (pid)
        {
                exit(0);
        }

        if (chdir("/") < 0)
        {
                syslog(LOG_ERR, "%s:chdir:%m", __func__);
                exit(1);
        }

        if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        {
                syslog(LOG_ERR, "%s:getrlimit:RLIMIT_NOFILE:%m", __func__);
                exit(1);
        }

        if (rl.rlim_max == RLIM_INFINITY)
        {
                rl.rlim_max = 1024;
        }
        for (unsigned int i = 0; i < rl.rlim_max; i++)
        {
                close(i);
        }

        fd0 = open("dev/null", O_RDWR);
        fd1 = dup(0);
        fd2 = dup(0);

        if (fd0 != 0 || fd1 != 1 || fd2 != 2)
        {
                syslog(LOG_ERR, "fd: %d %d %d", fd0, fd1, fd2);
                exit(1);
        }
}
