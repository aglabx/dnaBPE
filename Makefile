CXX=g++
CXXFLAGS=-std=c++2a -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive  -O3 -rdynamic
LDLIBS=

TARGET=bin/bpe.v2.exe
SRCS=nlohmann/json.hpp src/tokens.hpp src/tokens_model.hpp src/readers.hpp src/preprocess.hpp src/core.hpp src/output.hpp src/bpe.v2.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDLIBS) -o $(TARGET)

.PHONY: clean

clean:
	rm -f $(TARGET)

# sudo apt install libtbb-dev
# g++ -std=c++2a -pthread -static -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -O3 -rdynamic json.hpp bpe.cpp -o bpe.exe 
