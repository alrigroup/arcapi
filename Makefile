CC = gcc
CFLAGS = -Wall -g -pthread -w
LIBS = -lssl -lcrypto -lpq

all: arc_server core

arc_server:
	$(MAKE) -C ar-ws

core:
	$(CC) ar-core/main.c -o core $(CFLAGS)

run: all
	chmod +x core arc_server
	./core

clean:
	$(MAKE) -C ar-ws clean
	rm -f core arc_server

.PHONY: all clean run arc_server core
