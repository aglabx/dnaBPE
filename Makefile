CXX=g++
CXXFLAGS=-std=c++17 -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -O3 -rdynamic
CXXFLAGS_DEV=-std=c++17 -pthread -Wall -O1 
LDFLAGS=-g
LDLIBS=

TARGET=bin/bpe.exe
TARGET_DEV=bin/bpe.dev.exe
TARGET_SLOW=bin/bpe.slow.exe
TARGET_FAST=bin/bpe.fast.exe

SRCS=nlohmann/json.hpp src/tokens.hpp src/tokens_model.hpp src/readers.hpp src/preprocess.hpp src/core.hpp src/output.hpp src/subcontainers.hpp src/container.hpp src/bpe.v3.cpp

SRCS_SLOW=nlohmann/json.hpp src/tokens.hpp src/tokens_model.hpp src/readers.hpp src/preprocess.hpp src/core.hpp src/output.hpp src/subcontainers.hpp src/container.hpp src/bpe.v2.cpp

all: $(TARGET) $(TARGET_DEV) $(TARGET_SLOW)

prod: $(TARGET)

dev: $(TARGET_DEV)

slow: $(TARGET_SLOW)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDLIBS) -o $(TARGET)
	cp $(TARGET) $(TARGET_FAST)

$(TARGET_DEV): $(SRCS)
	$(CXX) $(CXXFLAGS_DEV) $(SRCS) $(LDLIBS) -o $(TARGET_DEV) $(LDFLAGS) 

$(TARGET_SLOW): $(SRCS_SLOW)
	git checkout e351f3f
	$(CXX) $(CXXFLAGS_DEV) $(SRCS) $(LDLIBS) -o $(TARGET_SLOW)
	git checkout master

.PHONY: all prod dev clean slow

clean:
	rm -f $(TARGET) $(TARGET_SLOW) $(TARGET_DEV) $(TARGET_FAST)
