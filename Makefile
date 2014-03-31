all: snc

snc: snc.c
	gcc snc.c -o snc
clean:
	rm -rf *o snc
