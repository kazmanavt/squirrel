# ALGO_VFUZ1
.EXPORT_ALL_VARIABLES:

OBJS = $(subst .c,.o,$(wildcard *.c))
CFLAGS += -Ilibfuzzy -m32  -fPIC
LDFLAGS  +=  -Wl,-m -Wl,elf_i386 -m32 -fPIC 
LIBS += -lstdc++ -lm  -lfuzzylite
LIB = libfuzzy/libfuzzy.o 
CC = gcc


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
