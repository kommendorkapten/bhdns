CC       = gcc
CFLAGS   = -D_POSIX_C_SOURCE=200112L -g -m64 -O3
CF_gcc   = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_clang = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_c99   = -v -errwarn -mt -xalias_level=std
CFLAGS  += $(CF_$(CC))

LNET     =
LFLAGS   = $(LNET)

DIRS  = bin
OBJS = bhd_cfg.o bhd_srv.o bhd_dns.o bhd_bl.o

.POSIX:
.PHONY: clean
.PHONY: all
.PHONY: libvendor

########################################################################

all: bin/bhdns

dirs: $(DIRS)

libvendor:
	cd vendor; CC=$(CC) CFLAGS="$(CFLAGS)" $(MAKE) -e

$(DIRS):
	mkdir $(DIRS)

clean:
	rm -rf bin/* *.o vendor/*.o vendor/libvendor.a

bin/bhdns: bhd.c $(OBJS) libvendor
	$(CC) $(CFLAGS) bhd.c $(OBJS) -o $@ $(LFLAGS) vendor/libvendor.a

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
