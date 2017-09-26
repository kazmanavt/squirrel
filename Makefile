# Main squirrel makefile

.EXPORT_ALL_VARIABLES:

CC = gcc

CFLAGS  = -Wall -Wextra -I. -std=c99 -g -O0
LDFLAGS = -Wall -Wextra

.PHONY: clean distclean squirrel filters algos

all: squirrel filters algos

squirrel:
	make -C src
	@cp src/squirrel squirrel

filters:
	make -C src/filters
	@mkdir -p filters
	@( /bin/ls -1 src/filters/*/*.so; echo filters ) | xargs cp 

algos:
	make -C src/algos
	@mkdir -p algos
	@( /bin/ls -1 src/algos/*/*.so; echo algos ) | xargs cp 

clean:
	@make -C src clean
	@make -C src/filters clean
	@make -C src/algos clean

distclean: clean
	@rm -f squirrel
	@rm -rf filters
	@rm -rf algos
