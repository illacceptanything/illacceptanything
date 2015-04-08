CFLAGS?=-Wall

.PHONY: all
all: yo

yo: yo.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f yo
