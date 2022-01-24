CFLAGS= -fsanitize=address,undefined -Wall -std=c++17

server: server.cpp client
	g++ $(CFLAGS) server.cpp -o server 

client: client.cpp
	g++ $(CFLAGS) client.cpp -o client