CXX=g++
CXXFLAGS=-std=c++17 -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -O3 -rdynamic
CXXFLAGS_DEV=-std=c++17 -pthread -Wall -O1 
LDFLAGS=-g
LDLIBS=

TARGET=bin/bpe.exe
TARGET_DEV=bin/bpe.dev.exe
SRCS=nlohmann/json.hpp src/tokens.hpp src/tokens_model.hpp src/readers.hpp src/preprocess.hpp src/core.hpp src/output.hpp src/subcontainers.hpp src/container.hpp src/bpe.v3.cpp

all: $(TARGET) $(TARGET_DEV)

prod: $(TARGET)

dev: $(TARGET_DEV)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDLIBS) -o $(TARGET)

$(TARGET_DEV): $(SRCS)
	$(CXX) $(CXXFLAGS_DEV) $(SRCS) $(LDLIBS) -o $(TARGET_DEV) $(LDFLAGS) 

.PHONY: all prod dev clean

clean:
	rm -f $(TARGET)
