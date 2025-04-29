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

xvec_c3.a: xvec.c3 
	c3c static-lib xvec.c3 -o xvec_c3

xvec_c3.so: xvec.c3
	c3c dynamic-lib xvec.c3 -o xvec_c3