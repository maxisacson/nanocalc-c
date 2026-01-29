CC=gcc
CFLAGS_COMMON=-Wall -Wextra
CFLAGS_DBG=$(CFLAGS_COMMON) -g
CFLAGS=$(CFLAGS_COMMON) -Werror -O3
LIBS=-lm
LDFLAGS=
PROG=nc

debug: nc.c lexer.c parser.c evaler.c utils.c
	$(CC) $(CFLAGS_DBG) $(LIBS) $(LDFLAGS) $^ -o$(PROG)

release: nc.c lexer.c parser.c evaler.c utils.c
	$(CC) $(CFLAGS) $(LIBS) $(LDFLAGS) $^ -o$(PROG)

.PHONY: clean
clean:
	rm -rf nc
