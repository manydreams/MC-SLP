CC = gcc
CFLAGS = -DLOG_USE_COLOR
CFLAGS += -O3
CFLAGS += -std=gnu99
CFLAGS += -c

VPATH = log:network:util

ifeq ($(DEBUG), 1)
CFLAGS += -g
endif

main: main.c lib.a
	$(CC) $(CFLAGS) main.c -o main

lib.a: log.o network.o packets.o thread_pool.o types.o
	ar rcs $@ $^

.PHONY: clean
clean:
	@for file in $$(ls *.o); do rm $$file; done
	if [ -e lib.a ]; then rm lib.a; fi
	if [ -e main ]; then rm main; fi
