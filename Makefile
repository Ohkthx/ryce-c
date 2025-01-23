# Compiler
CC := clang

# Default compiler flags
CFLAGS := -Wall -Wextra -std=c2x -O2 -MMD -MP -fsanitize=address -lpthread

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
TESTS_DIR := tests
TESTS_OBJ_DIR := $(OBJ_DIR)/tests

# Target executable
TARGET := $(BIN_DIR)/app
TEST_TARGET := $(BIN_DIR)/test_args

# Source files
SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
TEST_SRCS := $(shell find $(TESTS_DIR) -type f -name '*.c')

# Header files (for precompiled headers)
HEADERS := $(shell find $(SRC_DIR) -type f -name '*.h')
HEADERS_GCH := $(patsubst $(SRC_DIR)/%.h,$(OBJ_DIR)/%.h.gch,$(HEADERS))

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TEST_OBJS := $(patsubst $(TESTS_DIR)/%.c,$(TESTS_OBJ_DIR)/%.o,$(TEST_SRCS))

# Include paths
INCLUDES := -I$(SRC_DIR)

# Libraries
LIBS :=
TEST_LIBS := -lcheck

# Default target
all: $(TARGET) $(HEADERS_GCH)

# Build application
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	@$(CC) $(CFLAGS) -o $@ $^

# Run application with arguments
.PHONY: run
run: $(TARGET)
	@$(TARGET) $(ARGS)

# Build tests
tests: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
	@mkdir -p $(BIN_DIR)
	@$(CC) $(CFLAGS) -o $@ $^ $(TEST_LIBS)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compile test files
$(TESTS_OBJ_DIR)/%.o: $(TESTS_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Compile headers into precompiled headers (.h.gch)
$(OBJ_DIR)/%.h.gch: $(SRC_DIR)/%.h
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Run tests
run_tests: tests
	@$(BIN_DIR)/test_args

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Debug build
debug: clean
	$(MAKE) all \
	  CFLAGS="$(filter-out -O%,$(CFLAGS)) -O0 -g"

# Include dependency files
-include $(OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)

.PHONY: all clean tests run_tests debug
