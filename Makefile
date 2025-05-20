BUILD_DIR=build/
OUTPUT_DIR=./
SLD_DIR=sdl/

.PHONY: build run clean

build: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR)
	cmake --install ${BUILD_DIR} --prefix ${OUTPUT_DIR}

$(BUILD_DIR)/build.ninja:
	cmake -B $(BUILD_DIR) -G Ninja

run: build
	cd $(OUTPUT_DIR) && ./paint

clean:
	cmake --build $(BUILD_DIR) --target clean