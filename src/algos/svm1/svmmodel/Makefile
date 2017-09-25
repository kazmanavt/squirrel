# ALGO_SVM1

LDFLAGS= -m32 -fPIC -lm -lstdc++ -lrt -lpthread -ldl

LIBSVM=../libsvm
CC = gcc
OBJS = $(subst .c,.o,$(wildcard *.c)) 
CFLAGS += -m32 -O0 -fPIC -I. -I.. -I../.. -I../../.. -I$(LIBSVM) -Wall -g -std=c99 -D_SVID_SOURCE
SQDIR=../../..

%.o: %.c
	$(CC) -c $(CFLAGS) $<

#OBJ= $(OBJS)    ../svm1_lib.o ../svm1_libsvm.o  $(SQDIR)/ext_rail.a
OBJ= $(OBJS)    ../svm1_lib.o ../svm1_libsvm.o  $(SQDIR)/nxjson.o $(SQDIR)/kz_erch.o $(SQDIR)/jconf.o $(SQDIR)/rail.o $(SQDIR)/simple_conf.o $(SQDIR)/fastlog.o $(SQDIR)/tsignals.o $(SQDIR)/ext.o
 

TARGET=svmmodel
	
$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(CFLAGS) $(OBJ) $(LDFLAGS)  
	@echo MakeCode completed


../svm1_lib.o: ../svm1_lib.c
	@cd ..; make


#../emu.o:  ../emu.c
#	 $(CC) -c $(CFLAGS) ../emu.c  -o ../emu.o 


../svm1_libsvm.o: $(LIBSVM)/libsvm.o

$(LIBSVM)/libsvm.o:
	@cd $(LIBSVM); make
	


clean:
	@rm -f *.o $(TARGET)
