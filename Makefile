CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

asm12v1.1: $(OBJS)
		$(CC) -o asm12v1.1 $(OBJS) $(LDFLAGS)