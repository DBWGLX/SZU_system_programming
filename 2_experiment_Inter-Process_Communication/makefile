all:client calcserver
client:client.c
	g++ -o $@ clientinfo.h $^
calcserver:calcserver.c
	g++ -o $@ clientinfo.h $^
.PHONY:clean
clean:
	rm -f client calcserver
