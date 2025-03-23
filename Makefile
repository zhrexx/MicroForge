IGNORE_DIRS = xam
SOURCES = $(shell find . -type d -name "$(IGNORE_DIRS)" -prune -o -name "*.c" -print)
BUILD_DIR = ./
EXECS = $(patsubst %.c,$(BUILD_DIR)%, $(notdir $(SOURCES)))
GIT_HASH := $(shell git rev-parse HEAD)

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -static -DGIT_HASH='"$(GIT_HASH)"'
RM = rm

.PHONY: all clean

all: $(EXECS)

$(BUILD_DIR)%: $(SOURCES)
	$(CC) $(CFLAGS) $(filter %/$(notdir $@).c, $(SOURCES)) -o $@

clean:
	-$(RM) -f $(EXECS) 2>/dev/null || true
