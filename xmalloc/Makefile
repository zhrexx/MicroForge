CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -O2
AR = ar
ARFLAGS = rcs

LIB = libxmalloc.a

SRC = xmalloc.c
OBJ = $(SRC:.c=.o)
HEADER = xmalloc.h

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

shared: CFLAGS += -fPIC
shared: $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -shared -o libxmalloc.so $(SRC)

clean:
	rm -f $(OBJ) $(LIB) libxmalloc.so
