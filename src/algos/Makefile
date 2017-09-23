# squirrel/src/algos
.EXPORT_ALL_VARIABLES:

DIRS = $(filter-out .,$(shell find . -maxdepth 1 -type d))

CFLAGS += -fpic -I../..
LDFLAGS += -fpic

.PHONY: $(DIRS) clean

all: $(DIRS)
	echo $(DIRS)
	@for i in $^; do make -C $$i; done

clean: $(DIRS)
	@for i in $^; do make -C $$i clean; done
