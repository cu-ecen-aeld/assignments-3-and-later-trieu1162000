CROSS_COMPILE :=
CC := gcc
ifeq ($(CROSS_COMPILE),aarch64-none-linux-gnu-)
    CC := $(CROSS_COMPILE)$(CC)
endif
CFLAGS := -Wall
DEPS := $(wildcard *.c)
OBJ := $(DEPS:.c=.o)

all: writer

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

writer: $(OBJ)
	$(CC) -o $@ $< $(CFLAGS) 

clean: 
	rm -rf writer *.o
