all: server 

server: server.o
	g++ -o server server.o

server.o: server.cpp server.h
	g++ -c server.cpp

clean: 
	rm -f *.o server 