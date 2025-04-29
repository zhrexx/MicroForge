CC = gcc
CFLAGS = -Wall -Wextra -g -ggdb -O2
AR = ar
ARFLAGS = rcs

LIB = libxlib.a

SRC = xlib.c
OBJ = $(SRC:.c=.o)
HEADER = xlib.h

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

shared: CFLAGS += -fPIC
shared: $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -shared -o libxlib.so $(SRC)

clean:
	rm -f $(OBJ) $(LIB) libxlib.so
