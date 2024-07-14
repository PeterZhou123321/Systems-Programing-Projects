CC=gcc
OBJ=spchk.o
PATHTEST=tests
CFLAGS=-fsanitize=address -g -Wall -Werror

spchk: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core spchk