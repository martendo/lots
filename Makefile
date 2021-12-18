.SUFFIXES:

CFLAGS ?= -Wall -Wextra -O3 -flto
LDLIBS := -lncurses

SRCS = $(wildcard src/*.c)

all: lots
.PHONY: all

clean:
	rm -f lots
	rm -rf obj
	rm -rf dep
.PHONY: clean

# Link objects into the final executable
lots: $(patsubst src/%.c,obj/%.o,$(SRCS))
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

# Compile C source files to object files and create dependency files
obj/%.o dep/%.d: src/%.c
	@mkdir -p obj/$(*D) dep/$(*D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I include -MMD -MP -MQ obj/$*.o -MQ dep/$*.d -MF dep/$*.d -c -o obj/$*.o $<

# Include dependencies except if cleaning
ifneq ($(MAKECMDGOALS),clean)
-include $(patsubst src/%.c,dep/%.d,$(SRCS))
endif
