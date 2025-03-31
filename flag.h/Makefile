CC = gcc
CFLAGS = -Wall -Wextra -g
AR = ar
ARFLAGS = rcs
LIB = libflag.a
SRCS = flag.c
OBJS = $(SRCS:.c=.o)
all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c flag.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
