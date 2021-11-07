# all:
# 	g++ -Wall --std=c++20 -ljack -g jack_proxy.cpp arduino-serial-lib.c -o jack_proxy

GCC=clang
CFLAGS=-Wall -std=gnu99
LDFLAGS=-ljack

GXX=clang++
CXXFLAGS=-Wall -std=c++2a -g
LDXXFLAGS=-ljack -lpthread

all: 
	make jackproxy-to-stdout
	make jackproxy-from-stdin
	make arduino-bridge
	make apc40-mapping
	make controller

arduino-bridge: \
	arduino-bridge.c \
	arduino-serial-lib.c

# apc40/apc40.o: apc40/apc40.cpp apc40/apc40.h apc40/controls.h

%: %.cpp
	$(GXX) -o $@ $^ $(CXXFLAGS) $(LDXXFLAGS)

%: %.c
	$(GCC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

# %.o: %.cpp
# 	$(GXX) -o $@ -c $^ $(CXXFLAGS)

clean:
	rm *.o
