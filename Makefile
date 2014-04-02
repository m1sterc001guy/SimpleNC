all: snc

snc: snc.c
	gcc -pthread snc.c -o snc
clean:
	rm -rf *o snc
