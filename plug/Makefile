CC=gcc
CFLAGS_COMMON=-Wall -Wextra -std=c23 -I.. -fPIC
CFLAGS_DBG=$(CFLAGS_COMMON) -g
CFLAGS=$(CFLAGS_COMMON) -Werror -O3
LIBS=-lm
LDFLAGS=

.PHONY: all
all: math.so

%.so: %.c
	$(CC) $(CFLAGS_DBG) -shared $^ -o $@

.PHONY: clean
clean:
	rm -rf *.so
