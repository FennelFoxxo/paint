BUILD_DIR=build/
SLD_DIR=sdl/

.PHONY: build clean

build: $(BUILD_DIR)/SDL3.dll

$(BUILD_DIR)/SDL3.dll: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	cmake -B $(BUILD_DIR) -G Ninja

clean:
	cmake --build $(BUILD_DIR) --target clean