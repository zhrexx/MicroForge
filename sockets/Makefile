CC = gcc
CFLAGS = -Wall -Wextra -g
AR = ar
ARFLAGS = rcs
LIB = libsockets.a
SRCS = sockets.c
OBJS = $(SRCS:.c=.o)
all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c sockets.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
