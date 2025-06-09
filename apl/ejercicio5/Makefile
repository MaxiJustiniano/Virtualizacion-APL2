CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic
LDFLAGS = 
TARGETS = servidor cliente

all: $(TARGETS)

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c $(LDFLAGS)

cliente: cliente.c
	$(CC) $(CFLAGS) -o cliente cliente.c $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o