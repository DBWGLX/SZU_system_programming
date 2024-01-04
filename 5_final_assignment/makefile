all:chatserver_wyx client
chatserver_wyx:chatserver_wyx.c
	g++ $^ info.h -o $@ -pthread
client:client.c
	g++ $^ info.h -o $@
.PHONY:
clean:
	rm -rf chatserver_wyx client wyx_register wyx_login wyx_sendmsg wyx_logout
