objs := queue.o context.o preempt.o uthread.o
targets := context.o uthread.o preempt.o libuthread.a queue.o

#test_queue.o uthread_yield.o

CC      := gcc
CFLAGS  := -Wall -Werror
CFLAGS  += -g
#PANDOC  := pandoc

ifneq ($(V),1)
Q = @
endif

all: $(targets)

# Include path

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

DEPFLAGS = -MMD -MF $(@:.o=.d)


libuthread.a: $(objs)
	@echo "CC $@"
	$(Q) ar rcs libuthread.a $@ $^
	#$(Q)$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	@echo "clean"
	$(Q)rm -f $(targets) $(deps) $(objs)
