CC ?= $(CROSS_COMPILE)gcc
TARGET ?= aesdsocket
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread

DEPS := $(wildcard *.c)
OBJ := $(DEPS:.c=.o)

all: aesdsocket

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

default: all

clean: 
	rm -rf aesdsocket *.o
