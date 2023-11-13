all: server client

server: server.o
	g++ -o server server.o

client : client.cpp
	g++ -o client client.cpp

server.o: server.cpp server.h
	g++ -c server.cpp

clean: 
	rm -f *.o server client