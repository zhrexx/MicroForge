SOURCES = $(shell find . -name '*.c')
BUILD_DIR = ./
EXECS = $(BUILD_DIR)$(basename $(notdir $(SOURCES)))

CC = gcc
CFLAGS = -Wall -Wextra

.PHONY: all clean
all: $(EXECS)

$(EXECS): %:
	$(CC) $(CFLAGS) $(filter %/$@.c, $(SOURCES)) -o $@

clean:
	rm -f $(EXECS)

