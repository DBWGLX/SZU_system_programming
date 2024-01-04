all:client server
client:client.c
	g++ -o $@ info.h $^
server:server.c
	g++ -o $@ info.h $^
.PHONY:clean
clean:
	rm -f client server FIFO_1 FIFO_2 FIFO_3
