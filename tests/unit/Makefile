# Unit tests for xinfc
#
# Build: make
# Run:   make test
# Clean: make clean

CC      ?= gcc
CFLAGS  := -Wall -Wextra -Werror -Wshadow -Wconversion -Wformat=2 -Wpedantic -O2
LDFLAGS := -lcmocka

SRC_DIR := ../../package/xinfc/src
SRCS    := $(SRC_DIR)/xinfc-utils.c $(SRC_DIR)/wifi.c
HDRS    := $(SRC_DIR)/wifi.h $(SRC_DIR)/xinfc-utils.h
TESTS   := test_wifi.c

BUILD_DIR := build_dir
TARGET    := $(BUILD_DIR)/test_wifi
XINFC     := $(BUILD_DIR)/xinfc-wsc
LOCK_TEST := $(BUILD_DIR)/test_lock

.PHONY: all clean test smoke

all: $(TARGET) $(LOCK_TEST) $(XINFC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(TESTS) $(SRCS) $(HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(TESTS) $(SRCS) $(LDFLAGS) -o $(TARGET)

$(LOCK_TEST): test_lock.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test_lock.c $(LDFLAGS) -o $(LOCK_TEST)

$(XINFC): $(SRC_DIR)/*.c $(SRC_DIR)/*.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -DXINFC_VERSION='"test"' -I$(SRC_DIR) $(SRC_DIR)/*.c -o $(XINFC)

test: $(TARGET) $(LOCK_TEST)
	./$(TARGET)
	./$(LOCK_TEST)

smoke: $(XINFC)
	../smoke/test-binary.sh $(XINFC) test

clean:
	rm -rf $(BUILD_DIR)
