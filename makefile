.PHONY : all clean

all: Debug/ Debug/remote Debug/remotesvr

clean:
	rm -fr Debug/

Debug/remote.o : remote.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/remotesvr.o : remotesvr.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/main.o : main.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/parameters.o : parameters.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/process.o: process.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/server.o: server.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/client.o: client.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/try_catch.o: try_catch.c
	gcc -c -g -O0 -Wall -Wextra -Werror -Wno-unused-parameter -D_DEBUG -D_DBG=1  -o $@ $<

Debug/:
	mkdir Debug/

Debug/remote: Debug/remote.o Debug/parameters.o Debug/process.o Debug/client.o Debug/try_catch.o
	gcc -o $@ $^

Debug/remotesvr: Debug/remotesvr.o Debug/parameters.o Debug/process.o Debug/server.o Debug/try_catch.o
	gcc -o $@ $^