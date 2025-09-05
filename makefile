BUILD_DIR = build
TARGET = mo
TEST_DIR = test
TEST_TARGET = test_osint

CC = gcc
CFLAGS = -Wall -I./include
LDFLAGS = -lmicrohttpd -lcjson

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): CMakeLists.txt
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..
	cd $(BUILD_DIR) && make

test: $(TEST_DIR)/$(TEST_TARGET)
	./$(TEST_DIR)/$(TEST_TARGET)

$(TEST_DIR)/$(TEST_TARGET): $(TEST_DIR)/test_osint.c src/pdrm.c src/sspi.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TEST_DIR)/$(TEST_TARGET)
	rm -f $(TEST_DIR)/*.o

.PHONY: all run clean test
