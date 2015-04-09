CFLAGS?=-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual -Wcast-align -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith

.PHONY: all

all: yo

yo: yo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f yo
