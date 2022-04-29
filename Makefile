web_server :   http_connect_1.o
	g++ -pthread -o web_server  http_connect_1.o

http_connect_1.o : thread_pool.h http_connect.h http_connect_1.c++
	g++ -c -pthread http_connect_1.c++

.PHONY : clean
clean :
	rm   http_connect_1.o
