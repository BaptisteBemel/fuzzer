CC=gcc
all: src/main.c
	$(CC) src/main.c -o fuzzer 
clean:
	rm -f fuzzer
	rm -f *.tar