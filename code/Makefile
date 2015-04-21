CFLAGS?=-Wall -Wextra -pedantic-errors -Werror -Wfatal-errors -Wcast-qual -Wcast-align -Wconversion -Wdouble-promotion -Wfloat-equal -Wshadow -Wpointer-arith
CPPFLAGS?=$(CFLAGS) -Woverloaded-virtual -Weffc++ -Wold-style-cast -Wunused -Wstrict-aliasing -Wstrict-overflow -Wunsafe-loop-optimizations -Wconversion -Wlogical-op -Wdisabled-optimization

all: ayylmao.o decss.o hw.o toxic.o yo.o yoda.o zefside.o simple-tictactoe.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

.PHONY: all
