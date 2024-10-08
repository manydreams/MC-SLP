CC = gcc
CFLAGS = -DLOG_USE_COLOR
CFLAGS += -O3
CFLAGS += -std=gnu99
CFLAGS += -c

ifeq ($(DEBUG), 1)
CFLAGS += -g
endif

all: log.o

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o
