CXX=g++
CXXFLAGS=-std=c++17 -pthread -O3 -march=native -ffast-math -funroll-loops -flto \
         -fno-signed-zeros -fno-trapping-math -ftree-vectorize
CXXFLAGS_DEBUG=-std=c++17 -pthread -g -O0 -Wall
CXXFLAGS_PROFILE=-std=c++17 -pthread -g -pg -O2
CXXFLAGS_COVERAGE=$(CXXFLAGS_DEBUG) -fprofile-arcs -ftest-coverage

# Directories
SRCDIR=src
BINDIR=bin

# Source files
COMMON_SRCS=$(SRCDIR)/global.cpp $(SRCDIR)/profiler.cpp
SRCS_V4=$(COMMON_SRCS) $(SRCDIR)/v4.cpp
HEADERS=$(SRCDIR)/global.hpp \
        $(SRCDIR)/reader.hpp \
        $(SRCDIR)/queue.hpp \
        $(SRCDIR)/profiler.hpp \
        $(SRCDIR)/linkedvector.hpp \
        $(SRCDIR)/bpe.hpp \
        $(SRCDIR)/robin_hood.h

# Include paths
GTEST_DIR=$(shell brew --prefix googletest)
JSON_DIR=$(shell brew --prefix nlohmann-json)
INCLUDES=-I. -I$(SRCDIR) -I$(GTEST_DIR)/include -I$(JSON_DIR)/include

# Output files
TARGET_V4=$(BINDIR)/bpe.v6.exe
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

# Add test source files (only .cpp files)
TEST_IMPLEMENTATION_SRCS=$(SRCDIR)/global.cpp \
                        $(SRCDIR)/profiler.cpp

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

# Add coverage output directory
COVERAGE_DIR=$(CURDIR)/coverage-data
BUILD_DIR=$(CURDIR)/build

coverage: CXXFLAGS=$(CXXFLAGS_DEBUG) -fprofile-arcs -ftest-coverage --coverage
coverage: clean-coverage $(TEST_TARGET)
	@mkdir -p coverage
	@mkdir -p $(COVERAGE_DIR)
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && \
	GCOV_PREFIX=$(COVERAGE_DIR) \
	GCOV_PREFIX_STRIP=0 \
	$(CXX) $(CXXFLAGS) $(INCLUDES) \
		$(addprefix $(CURDIR)/,$(TEST_SRCS)) \
		$(addprefix $(CURDIR)/,$(TEST_IMPLEMENTATION_SRCS)) \
		-o test \
		-L$(GTEST_DIR)/lib -lgtest -lgtest_main -pthread
	cd $(BUILD_DIR) && ./test
	lcov --capture \
		--directory $(BUILD_DIR) \
		 --directory $(SRCDIR) \
		--base-directory . \
		--gcov-tool $(CURDIR)/llvm-gcov \
		--ignore-errors gcov,path,inconsistent,deprecated,empty \
		--rc branch_coverage=1 \
		--output-file coverage/coverage.info
	lcov --remove coverage/coverage.info \
		'/usr/*' '/opt/*' '*robin_hood.h' \
		--output-file coverage/coverage.info \
		--gcov-tool $(CURDIR)/llvm-gcov \
		--rc branch_coverage=1 \
		--ignore-errors inconsistent
	genhtml coverage/coverage.info \
		--output-directory coverage/html \
		--rc branch_coverage=1 \
		--ignore-errors inconsistent,category \
		--legend
	@echo "Coverage report generated in coverage/html/index.html"

# Create llvm-gcov script with proper path
$(shell echo '#!/bin/bash\n/opt/homebrew/opt/llvm/bin/llvm-cov gcov "$$@"' > llvm-gcov)
$(shell chmod +x llvm-gcov)

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

$(TEST_TARGET): $(TEST_SRCS) $(TEST_DEPS) $(TEST_IMPLEMENTATION_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TEST_SRCS) $(TEST_IMPLEMENTATION_SRCS) -o $@ -L$(GTEST_DIR)/lib -lgtest -lgtest_main -pthread

# Run tests with verbose output
test-verbose: $(TEST_TARGET)
	$(TEST_TARGET) --gtest_print_time=1 --gtest_output=xml:test_results.xml

profile-run: $(TARGET_PROFILE)
	./$(TARGET_PROFILE) $(ARGS)
	gprof $(TARGET_PROFILE) gmon.out > profile_report.txt
	@echo "Profile report generated in profile_report.txt"

clean: clean-coverage
	rm -rf $(BINDIR)
	rm -f llvm-gcov
	rm -f coverage.profraw

clean-coverage:
	rm -rf coverage
	rm -rf $(COVERAGE_DIR)
	rm -rf $(BUILD_DIR)
	find . -type f -name '*.gcda' -delete
	find . -type f -name '*.gcno' -delete
	find . -type f -name '*.gcov' -delete

.PHONY: all debug profile profile-run clean test coverage clean-coverage
