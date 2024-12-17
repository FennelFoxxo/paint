BUILD_DIR=build/
OUTPUT_DIR=output/
SLD_DIR=sdl/

.PHONY: build run clean

build: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR)
	cmake --install ${BUILD_DIR} --prefix ${OUTPUT_DIR}

$(BUILD_DIR)/build.ninja:
	cmake -B $(BUILD_DIR) -G Ninja -DSDL_STATIC=ON

run: build
	output/crosshair.exe

clean:
	cmake --build $(BUILD_DIR) --target clean