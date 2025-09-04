BUILD_DIR = build
TARGET = mo

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): CMakeLists.txt
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..
	cd $(BUILD_DIR) && make

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
