CC = gcc
CFLAGS = -DLOG_USE_COLOR
CFLAGS += -O3
CFLAGS += -std=gnu99

VPATH = log:network:util

ifeq ($(DEBUG), 1)
CFLAGS += -g
CFLAGS += -DDEBUG
endif

main: main.c libslp.a
	$(CC) $(CFLAGS) $< -L. -lslp -o main

libslp.a: log.o network.o packets.o thread_pool.o types.o yyjson.o config.o base64.o queue.o
	ar rcs $@ $^

.PHONY: clean run
clean:
	@for file in $$(ls *.o); do rm $$file; done
	if [ -e libslp.a ]; then rm libslp.a; fi
	if [ -e main ]; then rm main; fi
run: main
	./main
