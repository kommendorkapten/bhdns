CC       = c99
CFLAGS   = -D_POSIX_C_SOURCE=200112L -g -m64 -O3
CF_GCC   = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_CLANG = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_C99   = -v -errwarn -mt -xalias_level=std
CFLAGS  += $(CF_C99)

LNET     =-lnsl -lsocket
#LNET     =
LFLAGS   = $(LNET)

VPATH = lib
DIRS  = bin

.PHONY: clean
.PHONY: all

########################################################################

all: bin/bhdns

dirs: $(DIRS)

$(DIRS):
	mkdir $(DIRS)

clean:
	rm -rf bin/* *.o

bin/bhdns: bhd.c bhd_cfg.o strutil.o bhd_srv.o bhd_dns.o bhd_bl.o hmap.o stack.o timing.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
