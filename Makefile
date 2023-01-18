CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

asm12v1.1: $(OBJS)
	$(CC) -o asm12v1.1 $(OBJS) $(LDFLAGS)

v1.1: asm12v1.1
	./asm12v1.1 v1.1.txt 0 > bin12v1.1.txt

clean:
	rm -f asm12v1.1 *.o
