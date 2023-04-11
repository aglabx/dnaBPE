CXX=g++
CXXFLAGS=-std=c++2a -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -O3 -rdynamic
LDFLAGS=
LDLIBS=


# CXX=g++
# CXXFLAGS=-std=c++17 -pthread -Wall -O1 
# LDFLAGS=-g 
# LDLIBS=

# CXX=g++
# CXXFLAGS=-std=c++2a -pthread -Wall
# LDFLAGS=-g
# LDLIBS=


TARGET=bin/bpe.v7p.exe
SRCS=nlohmann/json.hpp src/tokens.hpp src/tokens_model.hpp src/readers.hpp src/preprocess.hpp src/core.hpp src/output.hpp src/subcontainers.hpp src/container.hpp src/bpe.v3.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDLIBS) -o $(TARGET) $(LDFLAGS) 

.PHONY: clean

clean:
	rm -f $(TARGET)


# sudo apt install libtbb-dev
# g++ -std=c++2a -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -O3 -rdynamic json.hpp bpe.cpp -o bpe.exe 
# gcc -std=c++2a -pthread -g nlohmann/json.hpp src/tokens.hpp src/tokens_model.hpp src/readers.hpp src/preprocess.hpp src/core.hpp src/output.hpp src/container.hpp src/bpe.v3.cpp -o bin/bpe.vg.exe
