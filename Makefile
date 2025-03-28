IGNORE_DIRS = xam XLua
SOURCES = $(shell find . -type d \( -name xam -o -name XLua \) -prune -o -name "*.c" -print)
EXEC_NAMES = $(notdir $(SOURCES:.c=))
EXECS = $(addprefix ./,$(EXEC_NAMES))
GIT_HASH := $(shell git rev-parse HEAD 2>/dev/null || echo "unknown")
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -static -DGIT_HASH='"$(GIT_HASH)"'
RM = rm -f
.PHONY: all clean

all: $(EXECS)

define make-exec-rule
$(1): $(2)
	$$(CC) $$(CFLAGS) $$< -o $$@
endef

$(foreach src,$(SOURCES),\
    $(eval $(call make-exec-rule,\
        ./$(notdir $(src:.c=)),\
        $(src)\
    )))

clean:
	$(RM) $(EXECS)
