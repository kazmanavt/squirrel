# squirrel/src/filters/median

OBJS = $(subst .c,.o,$(wildcard *.c))

all: median.so

%.o: %.c
	$(CC) -c $(CFLAGS) $<

median.so: median.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	@rm -f *.o *.so
