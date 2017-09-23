# ALGO_VFUZ1

CC = gcc
CCPLUS = g++
OBJS = $(subst .c,.o,$(wildcard *.c)) $(subst .cpp,.o,$(wildcard *.cpp)) 
CFLAGS += -m32 -I.  -Wall -g  -I.. -I../.. -I../../.. -I/usr/local/include

%.o: %.c
	$(CC) -c $(CFLAGS) $<
%.o: %.cpp
	$(CCPLUS) -c $(CFLAGS) $<


libsvm.o: $(OBJS)
	ld -m elf_i386 -o libfuzzy.o -r $(OBJS)
	
all: libfuzzy.o


clean:
	@rm -f *.o
