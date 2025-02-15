all:chatserver_wyx client
chatserver_wyx:chatserver_wyx.cpp
	g++ $^ info.h -o $@ -pthread  -std=c++11
client:client.cpp
	g++ $^ info.h -o $@ -std=c++11
.PHONY:
clean:
	rm -rf chatserver_wyx client wyx_register wyx_login wyx_sendmsg wyx_logout
