CFLAGS?=-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual -Wcast-align -Wconversion -Wdouble-promotion -Wfloat-equal -Wshadow -Wpointer-arith

all: yo

yo: yo.c
	$(CC) $(CFLAGS) $< -o $@
