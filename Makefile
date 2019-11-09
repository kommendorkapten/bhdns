CC       = gcc
CFLAGS   = -D_POSIX_C_SOURCE=200112L -O3 -g -m64
CF_GCC   = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_CLANG = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
#CF_C99   = -v -errwarn -mt -xalias_level=std
CF_C99   = -v -mt -xalias_level=std
CFLAGS  += $(CF_GCC)

LNET     =-lnsl -lsocket
#LNET     =
LFLAGS = $(LNET)

VPATH = lib
DIRS = obj bin

.PHONY: clean
.PHONY: all
########################################################################

all: bin/bhdns

dirs: $(DIRS)

$(DIRS):
	mkdir $(DIRS)

clean:
	rm -rf bin/* obj/*

bin/bhdns: bhd.c obj/bhd_cfg.o obj/strutil.o obj/bhd_srv.o obj/bhd_dns.o
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
