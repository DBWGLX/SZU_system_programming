all: client.out

client.out:client.cpp
	g++ $^ -o $@ -std=c++17 -pthread -ljansson

.PHONY: clean
clean:
	rm -rf client.out