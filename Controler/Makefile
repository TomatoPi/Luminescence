# all:
# 	g++ -Wall --std=c++20 -ljack -g jack_proxy.cpp arduino-serial-lib.c -o jack_proxy

GCC=clang
CFLAGS=-Wall -std=gnu99
LDFLAGS=-ljack

GXX=clang++
CXXFLAGS=-Wall -std=c++2a -g
LDXXFLAGS=-ljack -lpthread

all:
	make controller

controller: jack-bridge.o mapper.o manager.o arduino-bridge.o

jack-bridge.o: jack-bridge.hpp thread-queue.hpp

mapper.o: mapper.hpp

manager.o: manager.hpp ../driver/state.h

arduino-bridge.o: arduino-bridge.hpp

%: %.cpp
	$(GXX) -o $@ $^ $(CXXFLAGS) $(LDXXFLAGS)

%: %.c
	$(GCC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: %.cpp
	$(GXX) -o $@ -c $< $(CXXFLAGS)

clean:
	rm *.o
