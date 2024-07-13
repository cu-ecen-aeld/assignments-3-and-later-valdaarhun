CC=$(CROSS_COMPILE)gcc

all: writer

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ -c $^

writer: writer.o	
	@$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	@$(RM) $(wildcard *.o) writer	