# ALGO_SVM1
.EXPORT_ALL_VARIABLES:

LDFLAGS += -lm -Wl,-m -Wl,elf_i386 -m32 -fPIC  -lstdc++
LIBS += -lstdc++ -lm
LIBSVM=libsvm
CC = gcc
OBJS = $(subst .c,.o,$(wildcard *.c))
CFLAGS += -I.  -Ilibsvm  -Wall -g -m32 -fPIC

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

# -- # ALGO_SVM1
# -- .EXPORT_ALL_VARIABLES:
# -- 
# -- LDFLAGS += -Wl,-m -Wl,elf_i386 -m32 -fPIC -lm  -lstdc++
# -- 
# -- LIBSVM=libsvm
# -- CC = gcc
# -- OBJS = $(subst .c,.o,$(wildcard *.c)) 
# -- CFLAGS += -Wl,-m -Wl,elf_i386 -m32 -fPIC -I.  -Ilibsvm  -Wall -g
# -- 
# -- %.o: %.c
# -- 	$(CC) -c $(CFLAGS) $<
# -- 	
# -- all: svm1.so libsvm.o
# -- 
# -- svm1.so: $(OBJS) $(LIBSVM)/libsvm.o
# -- 	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)
# -- 
# -- libsvm.o: $(LIBSVM)/libsvm.o
# -- 	mv $(LIBSVM)/libsvm.o svm1_libsvm.o
# -- 
# -- 
# -- $(LIBSVM)/libsvm.o:
# -- 	@cd $(LIBSVM); make
# -- 	
# -- 
# -- 
# -- clean:
# -- 	@rm -f *.o
