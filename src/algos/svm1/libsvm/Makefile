# ALGO_SVM1

CC = gcc
CCPLUS = g++
OBJS = $(subst .c,.o,$(wildcard *.c)) $(subst .cpp,.o,$(wildcard *.cpp)) 
CFLAGS +=  -I.  -Wall -g -m32 -fPIC

%.o: %.c
	$(CC) -c $(CFLAGS) $<
%.o: %.cpp
	$(CCPLUS) -c $(CFLAGS) $<


libsvm.o: $(OBJS)
	# ld -o libsvm.o -r  $(OBJS)
	ld  -m elf_i386 -o libsvm.o -r  $(OBJS)
	
all: libsvm.o


clean:
	@rm -f *.o
