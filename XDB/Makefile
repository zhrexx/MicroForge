CC = gcc
CFLAGS = -Wall -Wextra -g -O2
AR = ar
ARFLAGS = rcs

LIB = libxdb.a

SRC = xdb.c
OBJ = $(SRC:.c=.o)
HEADER = xdb.h

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

shared: CFLAGS += -fPIC
shared: $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -shared -o libxdb.so $(SRC)

clean:
	rm -f $(OBJ) $(LIB) libxdb.so
