MYCFLAGS = -g
CFLAGS = -Wall -std=gnu99 -D_GNU_SOURCE -no-pie $(MYCFLAGS)
LDFLAGS = -pthread
CPPFLAGS = -MMD

EXES = cache train target attack
OBJS = $(EXES:=.o)
DEPS = $(wildcard *.d)

all: $(EXES)
clean:
	-rm $(EXES) $(OBJS) 
-include $(DEPS)

cache train: evict.o
