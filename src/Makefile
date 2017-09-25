# squirrel/src

OBJS = $(subst .c,.o,$(wildcard *.c))
EXT_OBJS = $(subst .c,.o,$(wildcard ext*.c))
EXT_HEADERS = rail.h
INTERNAL_OBJS = $(filter-out $(EXT_OBJS),$(OBJS))
#OBJS = squirrel.o \
#       worker.o \
#       in.o \
#       balancer.o \
#       rail.o \
#       algos.o \
#       filters.o \
#       proto.o \
#       tsignals.o \
#       simple_net.o \
#       jconf.o \
#       nxjson.o \
#       util.o \
#       simple_conf.o \
#       fastlog.o \
#       kz_erch.o

CFLAGS += -m32 -fPIC -DCONFIGURED_CLOCK=CLOCK_REALTIME
LDFLAGS += -m32 -Wl,-m -Wl,elf_i386  -fPIC -rdynamic -lrt -lpthread -ldl -lm

.PHONY: clean

all: squirrel ext_rail.a

%.o: %.c
	$(CC) -c $(CFLAGS) $<
	
squirrel: $(INTERNAL_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

ext_rail.a: $(EXT_OBJS) nxjson.o kz_erch.o jconf.o rail.o simple_conf.o fastlog.o tsignals.o
	ar -rcs $@ $^

clean:
	@rm -f *.o squirrel ext_rail.a
