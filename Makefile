CXX=g++
CXXFLAGS=-std=c++17 -pthread -O3 -march=native -ffast-math -funroll-loops -flto \
         -fno-signed-zeros -fno-trapping-math -ftree-vectorize
CXXFLAGS_DEBUG=-std=c++17 -pthread -g -O0 -Wall
CXXFLAGS_PROFILE=-std=c++17 -pthread -g -pg -O2

# Directories
SRCDIR=src
BINDIR=bin

# Source files
COMMON_SRCS=$(SRCDIR)/global.cpp
SRCS_V4=$(COMMON_SRCS) $(SRCDIR)/v4.cpp
HEADERS=$(SRCDIR)/global.hpp \
        $(SRCDIR)/reader.hpp \
        $(SRCDIR)/queue.hpp \
        $(SRCDIR)/profiler.hpp \
        $(SRCDIR)/linkedvector.hpp \
        $(SRCDIR)/bpe.hpp \
        $(SRCDIR)/robin_hood.h

# Include paths
INCLUDES=-I. -I$(SRCDIR)

# Output files
TARGET_V4=$(BINDIR)/bpe.v5.exe
TARGET_DEBUG=$(BINDIR)/bpe.debug.exe
TARGET_PROFILE=$(BINDIR)/bpe.profile.exe

# Test files
TEST_SRCS=tests/test_linkedvector.cpp \
          tests/test_queue.cpp \
          tests/test_tokenizer.cpp

# Add test requirements
TEST_DEPS=$(SRCDIR)/linkedvector.hpp \
         $(SRCDIR)/queue.hpp \
         $(SRCDIR)/global.hpp \
         $(SRCDIR)/bpe.hpp \
         $(SRCDIR)/reader.hpp

TEST_TARGET=$(BINDIR)/tests

# Create bin directory if it doesn't exist
$(shell mkdir -p $(BINDIR))

# Default target
all: $(TARGET_V4)

# Debug build
debug: CXXFLAGS=$(CXXFLAGS_DEBUG)
debug: $(TARGET_DEBUG)

# Profile build
profile: $(TARGET_PROFILE)

# Add test target
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Build rules
$(TARGET_V4): $(SRCS_V4) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS_V4) -o $@

$(TARGET_DEBUG): $(SRCS_V4) $(HEADERS)
	$(CXX) $(CXXFLAGS_DEBUG) $(INCLUDES) $(SRCS_V4) -o $@

$(TARGET_PROFILE): $(SRCS_V4) $(HEADERS)
	$(CXX) $(CXXFLAGS_PROFILE) $(INCLUDES) $(SRCS_V4) -lprofiler -o $@

$(TEST_TARGET): $(TEST_SRCS) $(TEST_DEPS)
	$(CXX) $(CXXFLAGS_DEBUG) $(INCLUDES) $(TEST_SRCS) -o $@ -lgtest -lgtest_main -pthread

# Run tests with verbose output
test-verbose: $(TEST_TARGET)
	$(TEST_TARGET) --gtest_print_time=1 --gtest_output=xml:test_results.xml

profile-run: $(TARGET_PROFILE)
	./$(TARGET_PROFILE) $(ARGS)
	gprof $(TARGET_PROFILE) gmon.out > profile_report.txt
	@echo "Profile report generated in profile_report.txt"

clean:
	rm -rf $(BINDIR)

.PHONY: all debug profile profile-run clean test
