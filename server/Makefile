CC=$(CROSS_COMPILE)gcc

.PHONY: all
all: aesdsocket

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

aesdsocket: aesdsocket.o	
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	$(RM) $(wildcard *.o) aesdsocket	
