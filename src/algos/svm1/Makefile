# ALGO_SVM1
.EXPORT_ALL_VARIABLES:

LDFLAGS += -lm -lstdc++

LIBSVM=libsvm
CC = gcc
OBJS = $(subst .c,.o,$(wildcard *.c)) 
CFLAGS += -I.  -Ilibsvm  -Wall -g

%.o: %.c
	$(CC) -c $(CFLAGS) $<
	
all: svm1.so libsvm.o

svm1.so: $(OBJS) $(LIBSVM)/libsvm.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)

libsvm.o: $(LIBSVM)/libsvm.o
	mv $(LIBSVM)/libsvm.o svm1_libsvm.o


$(LIBSVM)/libsvm.o:
	@cd $(LIBSVM); make
	


clean:
	@rm -f *.o
