CC = gcc
CFLAGS = -Wall -Wextra -g -O2
AR = ar
ARFLAGS = rcs

LIB = libxevent.a

SRC = xevent.c
OBJ = $(SRC:.c=.o)
HEADER = xevent.h

all: $(LIB)

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

shared: CFLAGS += -fPIC
shared: $(SRC) $(HEADER)
	$(CC) $(CFLAGS) -shared -o libxevent.so $(SRC)

clean:
	rm -f $(OBJ) $(LIB) libxevent.so
