
CFLAGS=-Wall -g -std=c11
HEADERS=$(wildcard *.h)

all : simul
all : testing

simul: cpu.o systeme.o asm.o simul.o
	$(CC) $(CFLAGS) -o $@ $^

testing:
	@test -e make-private && ./make-private || true

distrib:
	@test -e package-private.sh && ./package-private.sh || true

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -vf *.o simul
