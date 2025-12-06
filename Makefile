# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Build directory
BUILD_DIR = build

# Core source files
CORE_SRCS = $(wildcard src/core/*.c)

# Detect platform:
ifeq ($(OS),Windows_NT)
    # Windows (MSYS/MinGW/WSL might differ)
    PLATFORM_SRCS = src/platform/platform_win.c
    EXECUTABLE = micro-editor.exe
else
    # Assume POSIX (Linux, macOS, BSD, etc)
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        # macOS
        PLATFORM_SRCS = src/platform/platform_posix.c
        EXECUTABLE = micro-editor
    else ifeq ($(UNAME_S),Linux)
        # Linux
        PLATFORM_SRCS = src/platform/platform_posix.c
        EXECUTABLE = micro-editor
    else
        # Default fallback to posix
        PLATFORM_SRCS = src/platform/platform_posix.c
        EXECUTABLE = micro-editor
    endif
endif

# Main source
MAIN_SRC = src/main.c

# All sources
SRCS = $(CORE_SRCS) $(PLATFORM_SRCS) $(MAIN_SRC)

# Object files path in build directory
OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Create build directories needed
$(shell mkdir -p $(BUILD_DIR)/core)
$(shell mkdir -p $(BUILD_DIR)/platform)

all: $(EXECUTABLE)

# Compile rules
$(BUILD_DIR)/core/%.o: src/core/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/platform/%.o: src/platform/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link executable
$(EXECUTABLE): $(OBJS)
	$(CC) $(OBJS) -o $@

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

# Optional run target
run: all
	./$(EXECUTABLE)
