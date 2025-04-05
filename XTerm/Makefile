CC = gcc
CFLAGS = -Wall -Wextra -g -O2
AR = ar
ARFLAGS = rcs

LIB = libxterm.a

SRC = xterm.c
OBJ = $(SRC:.c=.o)
HEADER = xterm.h

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

shared: CFLAGS += -fPIC
shared: $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -shared -o libxterm.so $(SRC)

clean:
	rm -f $(OBJ) $(LIB) libxterm.so
