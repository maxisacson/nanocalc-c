CC=gcc
CFLAGS_COMMON=-Wall -Wextra -std=c23
CFLAGS_DBG=$(CFLAGS_COMMON) -g
CFLAGS=$(CFLAGS_COMMON) -Werror -O3
LIBS=-lm
LDFLAGS=
PROG=nc
SRCS=nc.c lexer.c parser.c evaler.c utils.c

.PHONY: debug
debug: nc-dbg plug

.PHONY: release
release: nc plug

.PHONY: nc-dbg
nc-dbg: $(SRCS)
	$(CC) $(CFLAGS_DBG) $(LIBS) $(LDFLAGS) $^ -o$(PROG)

nc: $(SRCS)
	$(CC) $(CFLAGS) $(LIBS) $(LDFLAGS) $^ -o$(PROG)

.PHONY: clean
clean: plug-clean
	rm -rf nc

.PHONY: plug
plug:
	$(MAKE) -C plug

.PHONY: plug-clean
plug-clean:
	$(MAKE) -C plug clean
