objs := disk.o fs.o
targets := disk.o fs.o libfs.a
#targets := sem.o tps.o libuthread.a


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


libfs.a: $(objs)
	@echo "CC $@"
	$(Q) ar rcs libfs.a $@ $^
	#$(Q)$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	@echo "clean"
	#$(Q)rm -f $(targets) $(deps) $(objs)
	$(Q)rm -f $(targets)
