CROSS_COMPILE :=
CC := gcc
ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
    CC := $(CROSS_COMPILE)$(CC)
endif
CFLAGS := -Wall -Werror
DEPS := $(wildcard *.c)
OBJ := $(DEPS:.c=.o)

all: aesdsocket

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

aesdsocket: $(OBJ)
	$(CC) -o $@ $< $(CFLAGS) 

default: all

clean: 
	rm -rf aesdsocket *.o
