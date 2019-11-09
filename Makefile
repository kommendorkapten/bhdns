CC       = gcc
CFLAGS   = -D_POSIX_C_SOURCE=200112L -O3 -g
CF_GCC   = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_CLANG = -std=c99 -pedantic -fstrict-aliasing \
           -Wextra -Wall -Wstrict-aliasing -Wconversion -Wno-sign-conversion
CF_C99   = -v -errwarn -mt -xalias_level=std
CFLAGS  += $(CF_GCC)

#LNET     =-lnsl -lsocket
LNET     =
LD_FLAGS = $(LNET)

DIRS=obj bin

########################################################################

.PHONY: clean

dirs: $(DIRS)

$(DIRS):
	mkdir $(DIRS)

clean:
	rm -rf bin/*

bin/bhdns: main.c
	$(CC) $(CFLAGS) $^ -o $@ $(LFLAGS)
