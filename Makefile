TSH = ./tsh
TSHARGS = "-p"
CC = gcc
CFLAGS = -Wall -O2
FILES = $(TSH) ./myspin ./mysplit ./mystop ./myint  ./mykill ./helloworld

all: $(FILES)

tsh: tsh.o util.o jobs.o
	$(CC) $(CFLAGS) tsh.o util.o jobs.o -o tsh

handle: handle.o util.o
	$(CC) $(CFLAGS) handle.o util.o -o handle

# clean up
clean:
	rm -f $(FILES) *.o *~ *.bak *.BAK