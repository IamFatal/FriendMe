PORT=50473
CFLAGS = -DPORT=\$(PORT) -Wall -g -std=c99 -Werror

friends_server: friends_server.o process_args.o friends.o 
	gcc $(CFLAGS) -o friends_server friends_server.o process_args.o friends.o

process_args.o: process_args.c friends.h friends_server.h
	gcc $(CFLAGS) -c process_args.c

friends_server.o: friends_server.c friends.h friends_server.h
	gcc $(CFLAGS) -c friends_server.c

friends.o: friends.c friends.h
	gcc $(CFLAGS) -c friends.c

clean: 
	rm friends_server *.o
