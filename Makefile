CC=gcc
CFLAGS_DBG=-Wall -Wextra -g
CFLAGS=-Wall -Wextra -Werror -O3
PROG=nc

debug: nc.c lexer.c
	$(CC) $(CFLAGS_DBG) $^ -o$(PROG)

release: nc.c lexer.c
	$(CC) $(CFLAGS) $^ -o$(PROG)

.PHONY: clean
clean:
	rm -rf nc
