CC = gcc
CFLAGS = -Wall -Wextra -g -O2
AR = ar
ARFLAGS = rcs

LIB = libxvec.a

SRC = xvec.c
OBJ = $(SRC:.c=.o)
HEADER = xvec.h

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

shared: CFLAGS += -fPIC
shared: $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -shared -o libxvec.so $(SRC)

clean:
	rm -f $(OBJ) $(LIB) libxvec.so
