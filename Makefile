SOURCES = $(shell find . -name '*.c')
BUILD_DIR = ./
EXECS = $(BUILD_DIR)$(basename $(notdir $(SOURCES)))
GIT_HASH := $(shell git rev-parse HEAD)

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -static -DGIT_HASH='"$(GIT_HASH)"'
RM = rm

.PHONY: all clean
all: $(EXECS)

$(EXECS): %:
	$(CC) $(CFLAGS) $(filter %/$@.c, $(SOURCES)) -o $@

clean:
	$(RM) -f $(EXECS)
