# ALGO_VFUZ1

OBJS = $(subst .c,.o,$(wildcard *.c))
CFLAGS += -Ilibfuzzy -m32 
LDFLAGS += -m32 -lm -lstdc++ -lfuzzylite  -L/usr/local/lib
LIB = libfuzzy/libfuzzy.o

.PHONY: $(LIB) clean

all: vfuz1.so

%.o: %.c
	$(CC) -c $(CFLAGS) $<

vfuz1.so: $(OBJS) $(LIB)
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)

$(LIB):
	@make -C $(dir $(LIB))


#libfuzzy.o: $(LIBFUZZY)/libfuzzy.o
#	@mv $(LIBFUZZY)/libfuzzy.o libfuzzy.o


#$(LIBFUZZY)/libfuzzy.o:
#	@make -C $(LIBFUZZY)

clean:
	@make -C $(dir $(LIB)) clean
	@rm -f *.o *.so
