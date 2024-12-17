BUILD_DIR=build/
OUTPUT_DIR=output/
SLD_DIR=sdl_src/

.PHONY: build clean

build: $(OUTPUT_DIR)/crosshair.exe

$(OUTPUT_DIR)/crosshair.exe: $(BUILD_DIR)/crosshair.exe
	cmake --install ${BUILD_DIR} --prefix ${OUTPUT_DIR}

$(BUILD_DIR)/crosshair.exe: $(BUILD_DIR)/build.ninja
	cmake --build $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	cmake -B $(BUILD_DIR) -G Ninja -DSDL_STATIC=ON

clean:
	cmake --build $(BUILD_DIR) --target clean