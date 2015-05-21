build: server.c client.c
	gcc client.c -o client
	gcc server.c -o server

clean: server client
	rm server
	rm client
