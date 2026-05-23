CXX ?= clang
EXE ?= Cataphract

all:
	@echo "Building with CMake using compiler: $(CXX)"
	mkdir -p build
	cd build && cmake .. -DCMAKE_CXX_COMPILER="$(CXX)" -DCMAKE_BUILD_TYPE=Release
	cd build && cmake --build . --config Release --parallel
	cp build/Cataphract $(EXE) || cp build/Cataphract.exe $(EXE)

clean:
	rm -rf build $(EXE)