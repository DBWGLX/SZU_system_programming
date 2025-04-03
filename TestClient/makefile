all: client.out

client.out:chat.pb.cc client.cpp
	g++ $^ -o $@ -std=c++17 -pthread -lprotobuf

.PHONY: clean
clean:
	rm -rf client.out