# all:
# 	g++ -Wall --std=c++20 -ljack -g jack_proxy.cpp arduino-serial-lib.c -o jack_proxy

GCC=clang
CFLAGS=-Wall
LDFLAGS=

GXX=clang++
CXXFLAGS=-Wall -std=c++2a -g
LDXXFLAGS=-ljack -lpthread

all: 
	make -C jackproxy
	make -C netbridge

jack_proxy: \
	jack_proxy.cpp \
	arduino-serial-lib.c

# apc40/apc40.o: apc40/apc40.cpp apc40/apc40.h apc40/controls.h

%: %.cpp
	$(GXX) -o $@ $^ $(CXXFLAGS) $(LDXXFLAGS)

# %.o: %.c
# 	$(GXX) -o $@ -c $< $(CFLAGS) $(LDFLAGS)

# %.o: %.cpp
# 	$(GXX) -o $@ -c $^ $(CXXFLAGS)

clean:
	rm *.o
